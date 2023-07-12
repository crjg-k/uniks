/**
 * @file virtio_disk.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief // driver for qemu's virtio disk device.
 * uses qemu's mmio interface to virtio.
 * qemu ... -drive file=fs.img,if=none,format=raw,id=x0 \
 * -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
 * @version 0.1
 * @date 2023-04-25 10:46:35
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */

#include "virtio_disk.h"
#include <device/device.h>
#include <mm/memlay.h>
#include <mm/phys.h>
#include <platform/platform.h>
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>
#include <uniks/list.h>


static struct {
	struct spinlock_t virtio_disk_lock;
	struct list_node_t wait_list;

	/**
	 * @brief a set (not a ring) of DMA descriptors, with which the
	 * driver tells the device where to read and write individual
	 * disk operations.
	 * there are NUM descriptors.
	 * most commands consist of a list of a couple of these descriptors.
	 */
	struct virtq_desc_t *desc;

	/**
	 * @brief a ring in which the driver writes descriptor numbers
	 * that the driver would like the device to process.
	 * it only includes the head descriptor of each list.
	 * the ring has NUM elements.
	 */
	struct virtq_avail_t *avail;

	/**
	 * @brief a ring in which the device writes descriptor numbers that
	 * the device has finished processing (just the head of each list).
	 * there are NUM used ring entries.
	 */
	struct virtq_used_t *used;

	// our own book-keeping
	int8_t free[VIRTIO_DESC_NUM];	// is a descriptor free?
	uint16_t used_index;		// we've looked this far in used[2..NUM]

	/**
	 * @brief track info about in-flight operations, for use when completion
	 * interrupt arrives. indexed by first descriptor index of list.
	 */
	struct {
		struct blkbuf_t *buf;
		int8_t status;
	} info[VIRTIO_DESC_NUM];

	// disk command headers. one-for-one with descriptors, for convenience.
	struct virtio_blk_req_t ops[VIRTIO_DESC_NUM];
} disk;

void virtio_disk_init()
{
	uint32_t status = 0;

	initlock(&disk.virtio_disk_lock, "virtio_disk");

	assert(*VIRTIO_DISK_R(VIRTIO_MMIO_MAGIC_VALUE) == MMIO_MAGIC and
	       *VIRTIO_DISK_R(VIRTIO_MMIO_VERSION) == MMIO_VERSION and
	       *VIRTIO_DISK_R(VIRTIO_MMIO_DEVICE_ID) == MMIO_DISK_DEVICE_ID and
	       *VIRTIO_DISK_R(VIRTIO_MMIO_VENDOR_ID) == MMIO_VENDOR_ID);

	// reset device
	*VIRTIO_DISK_R(VIRTIO_MMIO_STATUS) = status;

	// set ACKNOWLEDGE status bit
	set_variable_bit(status, VIRTIO_CONFIG_S_ACKNOWLEDGE);
	// set DRIVER status bit
	set_variable_bit(status, VIRTIO_CONFIG_S_DRIVER);
	*VIRTIO_DISK_R(VIRTIO_MMIO_STATUS) = status;

	// negotiate features
	uint64_t features = *VIRTIO_DISK_R(VIRTIO_MMIO_DEVICE_FEATURES);
	clear_variable_bit(features, 1 << VIRTIO_BLK_F_RO);
	clear_variable_bit(features, 1 << VIRTIO_BLK_F_SCSI);
	clear_variable_bit(features, 1 << VIRTIO_BLK_F_CONFIG_WCE);
	clear_variable_bit(features, 1 << VIRTIO_BLK_F_MQ);
	clear_variable_bit(features, 1 << VIRTIO_F_ANY_LAYOUT);
	clear_variable_bit(features, 1 << VIRTIO_RING_F_EVENT_IDX);
	clear_variable_bit(features, 1 << VIRTIO_RING_F_INDIRECT_DESC);
	*VIRTIO_DISK_R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

	// tell device that feature negotiation is complete
	set_variable_bit(status, VIRTIO_CONFIG_S_FEATURES_OK);
	*VIRTIO_DISK_R(VIRTIO_MMIO_STATUS) = status;

	// re-read status to ensure FEATURES_OK is set
	status = *VIRTIO_DISK_R(VIRTIO_MMIO_STATUS);
	assert(get_variable_bit(status, VIRTIO_CONFIG_S_FEATURES_OK));

	// initialize queue 0
	*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_SEL) = 0;
	// ensure queue 0 is not in use
	assert(!*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_READY));

	// check max queue size
	uint32_t max = *VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_NUM_MAX);
	assert(max != 0);
	assert(max >= VIRTIO_DESC_NUM);

	// allocate and zero queue memory
	disk.desc = pages_alloc(1);
	disk.avail = pages_alloc(1);
	disk.used = pages_alloc(1);
	assert(disk.desc != NULL and disk.avail != NULL and disk.used != NULL);
	memset(disk.desc, 0, PGSIZE);
	memset(disk.avail, 0, PGSIZE);
	memset(disk.used, 0, PGSIZE);

	// set queue size
	*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_NUM) = VIRTIO_DESC_NUM;

	// write physical addresses
	*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64_t)disk.desc;
	*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_DESC_HIGH) =
		get_high_32bit((uint64_t)disk.desc);
	*VIRTIO_DISK_R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64_t)disk.avail;
	*VIRTIO_DISK_R(VIRTIO_MMIO_DRIVER_DESC_HIGH) =
		get_high_32bit((uint64_t)disk.avail);
	*VIRTIO_DISK_R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64_t)disk.used;
	*VIRTIO_DISK_R(VIRTIO_MMIO_DEVICE_DESC_HIGH) =
		get_high_32bit((uint64_t)disk.used);

	// queue is ready
	*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

	// all NUM descriptors start with free status
	for (int32_t i = 0; i < VIRTIO_DESC_NUM; i++)
		disk.free[i] = 1;

	// tell device we're completely ready
	set_variable_bit(status, VIRTIO_CONFIG_S_DRIVER_OK);
	*VIRTIO_DISK_R(VIRTIO_MMIO_STATUS) = status;

	// software-layer initialize
	INIT_LIST_HEAD(&disk.wait_list);
	char virtio_disk_name[] = "virtio_disk";
	struct device_t *devptr = &devices[VIRTIO_IRQ];
	device_install(DEV_BLOCK, DEV_EXTERNAL_STORAGE, &disk, virtio_disk_name,
		       0, do_virtio_disk_interrupt, NULL, NULL, NULL, devptr);
	// plic.c and trap.c arrange for interrupts from VIRTIO_IRQ.
}

// find a free descriptor, mark it non-free, then return its index
static int32_t alloc_desc()
{
	for (int32_t i = 0; i < VIRTIO_DESC_NUM; i++) {
		if (disk.free[i]) {
			disk.free[i] = 0;
			return i;
		}
	}
	return -1;
}

// mark a descriptor as free
static void free1_descriptor(int32_t i)
{
	assert(i < VIRTIO_DESC_NUM);
	assert(!disk.free[i]);
	disk.desc[i].addr = disk.desc[i].len = disk.desc[i].flags =
		disk.desc[i].next = 0;
	disk.free[i] = 1;
	// wakeup(&disk.free[0]);
}

// free a list of descriptors
static void free_descriptor_list(int32_t i)
{
	while (1) {
		int32_t flag = disk.desc[i].flags;
		int32_t nxt = disk.desc[i].next;
		free1_descriptor(i);
		if (get_variable_bit(flag, VRING_DESC_F_NEXT))
			i = nxt;
		else
			break;
	}
}

/**
 * @brief allocate three descriptors (they need not be contiguous).
 * disk transfers always use three descriptors.
 * @param index
 * @return int32_t
 */
static int32_t alloc3_desc(int32_t *index)
{
	for (int32_t i = 0; i < 3; i++) {
		index[i] = alloc_desc();
		if (index[i] < 0) {
			for (int32_t j = 0; j < i; j++)
				free1_descriptor(index[j]);
			return -1;
		}
	}
	return 0;
}

static void virtio_disk_rw(struct blkbuf_t *buf, int32_t write)
{
#define SECTORSIZE 512
	uint64_t sector = buf->b_blkno * (BLKSIZE / SECTORSIZE);

	acquire(&disk.virtio_disk_lock);

	/**
	 * @brief the spec's Section 5.2 says that legacy block operations use
	 * three descriptors: one for type/reserved/sector, one for the data,
	 * one for a 1-byte status result.
	 */

	// allocate the three descriptors.
	int32_t index[3];
	while (1) {
		if (alloc3_desc(index) == 0) {
			break;
		}
		proc_block(myproc(), &disk.wait_list, TASK_BLOCK);
		release(&disk.virtio_disk_lock);
		sched();
		acquire(&disk.virtio_disk_lock);
	}

	// format the three descriptors. And qemu's virtio-blk.c reads them.

	struct virtio_blk_req_t *buf0 = &disk.ops[index[0]];

	if (write)
		buf0->type = VIRTIO_BLK_T_OUT;	 // write the disk
	else
		buf0->type = VIRTIO_BLK_T_IN;	 // read the disk
	buf0->reserved = 0;
	buf0->sector = sector;

	disk.desc[index[0]].addr = (uint64_t)buf0;
	disk.desc[index[0]].len = sizeof(struct virtio_blk_req_t);
	disk.desc[index[0]].flags = VRING_DESC_F_NEXT;
	disk.desc[index[0]].next = index[1];

	disk.desc[index[1]].addr = (uint64_t)buf->b_data;
	disk.desc[index[1]].len = BLKSIZE;
	if (write)
		disk.desc[index[1]].flags =
			VRING_DESC_F_READ;   // device reads buf->b_data
	else
		disk.desc[index[1]].flags =
			VRING_DESC_F_WRITE;   // device writes buf->b_data
	set_variable_bit(disk.desc[index[1]].flags, VRING_DESC_F_NEXT);
	disk.desc[index[1]].next = index[2];

	disk.info[index[0]].status = 0xff;   // device writes 0 on success
	disk.desc[index[2]].addr = (uint64_t)&disk.info[index[0]].status;
	disk.desc[index[2]].len = 1;
	disk.desc[index[2]].flags =
		VRING_DESC_F_WRITE;   // device writes the status
	disk.desc[index[2]].next = 0;

	// record struct buf for do_virtio_disk_interrupt().
	buf->b_disk = 1;
	disk.info[index[0]].buf = buf;

	// tell the device the first index in our list of descriptors.
	disk.avail->ring[disk.avail->index % VIRTIO_DESC_NUM] = index[0];

	__sync_synchronize();

	// tell the device another avail ring entry is available.
	disk.avail->index += 1;	  // not % NUM ...

	__sync_synchronize();

	*VIRTIO_DISK_R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0;	// value is queue number

	// Wait for do_virtio_disk_interrupt() to say request has finished.
	while (buf->b_disk == 1) {
		proc_block(myproc(), &disk.wait_list, TASK_BLOCK);
		release(&disk.virtio_disk_lock);
		sched();
		acquire(&disk.virtio_disk_lock);
	}

	disk.info[index[0]].buf = NULL;
	free_descriptor_list(index[0]);

	release(&disk.virtio_disk_lock);
}

void do_virtio_disk_interrupt(void *ptr)
{
	acquire(&disk.virtio_disk_lock);

	/**
	 * @brief the device won't raise another interrupt until we tell it
	 * we've seen this interrupt, which the following line does. this may
	 * race with the device writing new entries to the "used" ring, in which
	 * case we may process the new completion entries in this interrupt, and
	 * have nothing to do in the next interrupt, which is harmless.
	 */
	*VIRTIO_DISK_R(VIRTIO_MMIO_INTERRUPT_ACK) =
		*VIRTIO_DISK_R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;

	__sync_synchronize();

	/**
	 * @brief the device increments disk.used->index when it adds an entry
	 * to the used ring.
	 */
	while (disk.used_index != disk.used->index) {
		__sync_synchronize();
		int32_t id =
			disk.used->ring[disk.used_index % VIRTIO_DESC_NUM].id;

		assert(disk.info[id].status == 0);

		struct blkbuf_t *buf = disk.info[id].buf;
		buf->b_disk = 0;   // disk is done with buf
		buf->b_valid = 1;
		struct proc_t *p = proc_unblock(&disk.wait_list);
		release(&pcblock[p->pid]);
		disk.used_index += 1;
	}

	release(&disk.virtio_disk_lock);
}

void virtio_disk_read(struct blkbuf_t *bb)
{
	virtio_disk_rw(bb, 0);
}

void virtio_disk_write(struct blkbuf_t *bb)
{
	assert(bb->b_dirty == 1);
	virtio_disk_rw(bb, 1);
	bb->b_dirty = 0;
}