#ifndef __FS_BLKBUF_H__
#define __FS_BLKBUF_H__

#include <fs/fs.h>
#include <sync/mutex.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>


#define _hashfn(dev, block) (((uint32_t)(dev ^ block)) % BLKBUF_HASH_PRIME)
#define hash(dev, block)    (hash_table[_hashfn(dev, block)])

struct blkbuf_t {
	uint32_t b_dev;
	uint32_t b_blkno;
	int8_t b_valid;	    // is mapping to a disk block?
	int8_t b_disk;	    // does this block wait for a disk request done?
	int8_t b_dirty;	    // is this block had been modified?
	uint32_t b_count;   // record that if a process occupy it

	struct mutex_t b_mtx;

	struct list_node_t hash_node;	// hash bucket list
	struct list_node_t free_node;	// LRU cache list

	char b_data[BLKSIZE];
};

struct free_list_head_t {
	struct list_node_t free_head;
	struct list_node_t wait_list;
	struct spinlock_t free_list_head_lock;
};

struct hash_bucket_t {
	struct list_node_t bucket_head;
	struct spinlock_t hash_lock;
};

extern struct blkbuf_t blkbuf[];
extern struct free_list_head_t free_list_head;
extern struct hash_bucket_t hash_table[];

void blkbuf_init();
struct blkbuf_t *getblk(dev_t dev, uint32_t blkno);
void brelease(struct blkbuf_t *bb);
struct blkbuf_t *blk_read(dev_t dev, uint32_t blockno);
void blk_write(struct blkbuf_t *bb);


#endif /* !__FS_BLKBUF_H__ */