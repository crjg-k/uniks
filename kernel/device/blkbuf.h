#ifndef __KERNEL_DEVICE_BLKBUF_H__
#define __KERNEL_DEVICE_BLKBUF_H__


#include <sync/mutex.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>


struct blkbuf_t {
	uint32_t b_dev;
	uint32_t b_blkno;
	int8_t b_valid;	    // is mapping to a disk block?
	int8_t b_disk;	    // does this block wait for a disk request done?
	int8_t b_dirty;	    // is this block had been modified?
	int8_t b_hashed;    // is this in correct hash location?
	uint32_t b_count;   // record that if a process occupy it

	struct mutex_t b_mtx;

	struct list_node_t hash_node;	// hash bucket list
	struct list_node_t free_node;	// LRU cache list
	// waiting for disk rw(corresponding to b_disk)
	struct list_node_t disk_wait_list;

	char *b_data;	// dynamically allocate
};

struct blk_cache_t {
	struct spinlock_t lock;
	struct blkbuf_t blkbuf[NBUF];
	struct list_node_t free_list;	// free_list
	struct list_node_t wait_list;	// wait for free_list
	struct list_node_t hash_bucket_table[HASH_TABLE_PRIME];
};

extern struct blk_cache_t blk_cache;


void blk_init();
void blk_release(struct blkbuf_t *bb);
struct blkbuf_t *blk_read(dev_t dev, uint32_t blockno);
void blk_write_over(struct blkbuf_t *bb);
void blk_sync_all();


#endif /* !__KERNEL_DEVICE_BLKBUF_H__ */