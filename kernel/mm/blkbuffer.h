#ifndef __KERNEL_MM_BLKBUFFER_H__
#define __KERNEL_MM_BLKBUFFER_H__

#include <defs.h>
#include <fs/fs.h>
#include <list.h>
#include <sync/spinlock.h>

struct blkbuf {
	int32_t valid;	 // has data been read from disk?
	int32_t disk;	 // does disk "own" buffer?
	uint32_t device;
	uint32_t blknum;
	// todo: accomplish the sleeplock with more complicated scheduler
	// struct sleeplock lock;
	uint32_t refcnt;
	uint8_t data[BLKSIZE];
	// LRU cache list
	struct list_node node;
};

void buffer_init();

#endif /* !__KERNEL_MM_BLKBUFFER_H__ */