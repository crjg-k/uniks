#ifndef __KERNEL_DEVICE_VIRTIO_DISK_H__
#define __KERNEL_DEVICE_VIRTIO_DISK_H__

/**
 * @brief virtio device definitions.
 * for both the mmio interface, and virtio descriptors.
 * only tested with qemu.
 *
 * the virtio specification:
 * https://docs.oasis-open.org/virtio/virtio/v1.2/virtio-v1.2.pdf
 *
 */

#include <fs/blkbuf.h>
#include <platform/platform.h>
#include <uniks/defs.h>


#define MMIO_MAGIC	    (0x74726976)
#define MMIO_VERSION	    (2)
#define MMIO_NET_DEVICE_ID  (1)
#define MMIO_DISK_DEVICE_ID (2)
#define MMIO_VENDOR_ID	    (0x554d4551)

/**
 * virtio mmio control registers, mapped starting at 0x10001000 from qemu
 * virtio_mmio.h
 */
#define VIRTIO_MMIO_MAGIC_VALUE	    (0x000)   // 0x74726976
#define VIRTIO_MMIO_VERSION	    (0x004)   // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID	    (0x008)   // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID	    (0x00c)   // 0x554d4551
#define VIRTIO_MMIO_DEVICE_FEATURES (0x010)
#define VIRTIO_MMIO_DRIVER_FEATURES (0x020)
#define VIRTIO_MMIO_QUEUE_SEL	    (0x030)   // select queue, write-only
#define VIRTIO_MMIO_QUEUE_NUM_MAX \
	0x034	// max size of current queue, read-only
#define VIRTIO_MMIO_QUEUE_NUM	     (0x038)   // size of current queue, write-only
#define VIRTIO_MMIO_QUEUE_READY	     (0x044)   // ready bit
#define VIRTIO_MMIO_QUEUE_NOTIFY     (0x050)   // write-only
#define VIRTIO_MMIO_INTERRUPT_STATUS (0x060)   // read-only
#define VIRTIO_MMIO_INTERRUPT_ACK    (0x064)   // write-only
#define VIRTIO_MMIO_STATUS	     (0x070)   // read/write
#define VIRTIO_MMIO_QUEUE_DESC_LOW \
	(0x080)	  // physical address for descriptor table, write-only
#define VIRTIO_MMIO_QUEUE_DESC_HIGH (0x084)
#define VIRTIO_MMIO_DRIVER_DESC_LOW \
	(0x090)	  // physical address for available ring, write-only
#define VIRTIO_MMIO_DRIVER_DESC_HIGH (0x094)
#define VIRTIO_MMIO_DEVICE_DESC_LOW \
	(0x0a0)	  // physical address for used ring, write-only
#define VIRTIO_MMIO_DEVICE_DESC_HIGH (0x0a4)

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE (1)
#define VIRTIO_CONFIG_S_DRIVER	    (2)
#define VIRTIO_CONFIG_S_DRIVER_OK   (4)
#define VIRTIO_CONFIG_S_FEATURES_OK (8)

// device feature bits
#define VIRTIO_BLK_F_RO		    (5)	 /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI	    (7)	 /* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE	    (11) /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ		    (12) /* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT	    (27)
#define VIRTIO_RING_F_INDIRECT_DESC (28)
#define VIRTIO_RING_F_EVENT_IDX	    (29)

// generate the address of virtio mmio register r
#define VIRTIO_DISK_R(r) ((volatile uint32_t *)(VIRTIO0 + (r)))

// the number of virtio descriptors must be a power of 2
#define VIRTIO_DESC_NUM (32)

// a single descriptor, from the spec.
struct virtq_desc_t {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
};
#define VRING_DESC_F_NEXT  (1)   // chained with another descriptor
#define VRING_DESC_F_READ  (0)   // device reads
#define VRING_DESC_F_WRITE (2)   // device writes (vs read)

// the (entire) avail ring, from the spec.
struct virtq_avail_t {
	uint16_t flags;			  // always zero
	uint16_t index;			  // driver will write ring[index] next
	uint16_t ring[VIRTIO_DESC_NUM];	  // descriptor numbers of chain heads
	uint16_t unused;
};

/**
 * @brief one entry in the "used" ring, with which the
 * device tells the driver about completed requests.
 */
struct virtq_used_elem_t {
	uint32_t id;   // index of start of completed descriptor chain
	uint32_t len;
};

struct virtq_used_t {
	uint16_t flags;	  // always zero
	uint16_t index;	  // device increments when it adds a ring[] entry
	struct virtq_used_elem_t ring[VIRTIO_DESC_NUM];
};


/**
 * @brief these are specific to virtio block devices, e.g. disks, described in
 * Section 5.2 of the spec.
 */
#define VIRTIO_BLK_T_IN	 (0)   // read the disk
#define VIRTIO_BLK_T_OUT (1)   // write the disk

/**
 * @brief The format of the first descriptor in a disk request. To be followed
 * by two more descriptors containing the block, and a one-byte status.
 */
struct virtio_blk_req_t {
	uint32_t type;	 // VIRTIO_BLK_T_IN or ..._OUT
	uint32_t reserved;
	uint64_t sector;
};


void virtio_disk_init();
void virtio_disk_read(struct blkbuf_t *buf);
void virtio_disk_write(struct blkbuf_t *buf);
void do_virtio_disk_interrupt(void *ptr);


#endif /* !__KERNEL_DEVICE_VIRTIO_DISK_H__ */