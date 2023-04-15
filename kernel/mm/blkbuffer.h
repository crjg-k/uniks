#ifndef __KERNEL_MM_blkbuf_tFER_H__
#define __KERNEL_MM_blkbuf_tFER_H__

#include <fs/fs.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>

struct blkbuf_t {
	int32_t valid;	 // has data been read from disk?
	int32_t disk;	 // does disk "own" buffer?
	uint32_t device;
	uint32_t blknum;
	// todo: accomplish the sleeplock with more complicated scheduler
	// struct sleeplock lock;
	uint32_t refcnt;
	uint8_t data[BLKSIZE];
	// LRU cache list
	struct list_node_t node;
};

void buffer_init();

#endif /* !__KERNEL_MM_blkbuf_tFER_H__ */