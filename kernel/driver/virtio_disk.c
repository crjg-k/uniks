#include "virtio_disk.h"
#include <kassert.h>
#include <kstring.h>
#include <mm/memlay.h>
#include <mm/blkbuffer.h>
#include <sync/spinlock.h>


// generate the address of virtio mmio register r
#define virtio_disk_R(r) ((volatile uint32_t *)(VIRTIO0 + (r)))

extern void *kalloc();

static struct {
	struct spinlock virtio_disk_lock;

	/**
	 * @brief a set (not a ring) of DMA descriptors, with which the
	 * driver tells the device where to read and write individual
	 * disk operations.
	 * there are NUM descriptors.
	 * most commands consist of a list of a couple of these descriptors.
	 */
	struct virtq_desc *desc;

	/**
	 * @brief a ring in which the driver writes descriptor numbers
	 * that the driver would like the device to process.
	 * it only includes the head descriptor of each list.
	 * the ring has NUM elements.
	 */
	struct virtq_avail *avail;

	/**
	 * @brief a ring in which the device writes descriptor numbers that
	 * the device has finished processing (just the head of each list).
	 * there are NUM used ring entries.
	 */
	struct virtq_used *used;

	// our own book-keeping
	int8_t free[NUM];      // is a descriptor free?
	uint16_t used_index;   // we've looked this far in used[2..NUM].

	/**
	 * @brief track info about in-flight operations, for use when completion
	 * interrupt arrives. indexed by first descriptor index of list.
	 */
	struct {
		struct blkbuf *b;
		int8_t status;
	} info[NUM];

	// disk command headers. one-for-one with descriptors, for convenience.
	struct virtio_blk_req ops[NUM];
} disk;

void virtio_disk_init()
{
	uint32_t status = 0;

	initlock(&disk.virtio_disk_lock, "virtio_disk");

	assert(!(*virtio_disk_R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
		 *virtio_disk_R(VIRTIO_MMIO_VERSION) != 2 ||
		 *virtio_disk_R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
		 *virtio_disk_R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551));

	// reset device
	*virtio_disk_R(VIRTIO_MMIO_STATUS) = status;


	status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;	 // set ACKNOWLEDGE status bit
	status |= VIRTIO_CONFIG_S_DRIVER;	 // set DRIVER status bit
	*virtio_disk_R(VIRTIO_MMIO_STATUS) = status;

	// negotiate features
	uint64_t features = *virtio_disk_R(VIRTIO_MMIO_DEVICE_FEATURES);
	features &= ~(1 << VIRTIO_BLK_F_RO);
	features &= ~(1 << VIRTIO_BLK_F_SCSI);
	features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
	features &= ~(1 << VIRTIO_BLK_F_MQ);
	features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
	features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
	features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
	*virtio_disk_R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

	// tell device that feature negotiation is complete
	status |= VIRTIO_CONFIG_S_FEATURES_OK;
	*virtio_disk_R(VIRTIO_MMIO_STATUS) = status;

	// re-read status to ensure FEATURES_OK is set
	status = *virtio_disk_R(VIRTIO_MMIO_STATUS);
	assert((status & VIRTIO_CONFIG_S_FEATURES_OK));

	// initialize queue 0
	*virtio_disk_R(VIRTIO_MMIO_QUEUE_SEL) = 0;
	// ensure queue 0 is not in use
	assert(!*virtio_disk_R(VIRTIO_MMIO_QUEUE_READY));

	// check max queue size
	uint32_t max = *virtio_disk_R(VIRTIO_MMIO_QUEUE_NUM_MAX);
	assert(max != 0);
	assert(max >= NUM);

	// allocate and zero queue memory
	disk.desc = kalloc();
	disk.avail = kalloc();
	disk.used = kalloc();
	assert(disk.desc and disk.avail and disk.used);
	memset(disk.desc, 0, PGSIZE);
	memset(disk.avail, 0, PGSIZE);
	memset(disk.used, 0, PGSIZE);

	// set queue size
	*virtio_disk_R(VIRTIO_MMIO_QUEUE_NUM) = NUM;

	// write physical addresses
	*virtio_disk_R(VIRTIO_MMIO_QUEUE_DESC_LOW) = (uint64_t)disk.desc;
	*virtio_disk_R(VIRTIO_MMIO_QUEUE_DESC_HIGH) = (uint64_t)disk.desc >> 32;
	*virtio_disk_R(VIRTIO_MMIO_DRIVER_DESC_LOW) = (uint64_t)disk.avail;
	*virtio_disk_R(VIRTIO_MMIO_DRIVER_DESC_HIGH) =
		(uint64_t)disk.avail >> 32;
	*virtio_disk_R(VIRTIO_MMIO_DEVICE_DESC_LOW) = (uint64_t)disk.used;
	*virtio_disk_R(VIRTIO_MMIO_DEVICE_DESC_HIGH) =
		(uint64_t)disk.used >> 32;

	// queue is ready
	*virtio_disk_R(VIRTIO_MMIO_QUEUE_READY) = 0x1;

	// all NUM descriptors start out unused
	for (int32_t i = 0; i < NUM; i++)
		disk.free[i] = 1;

	// tell device we're completely ready
	status |= VIRTIO_CONFIG_S_DRIVER_OK;
	*virtio_disk_R(VIRTIO_MMIO_STATUS) = status;

	// plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ
}
