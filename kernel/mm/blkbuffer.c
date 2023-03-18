#include "blkbuffer.h"
#include <param.h>


struct spinlock bufcache_lock;
struct blkbuf blkbufcache[NBUF];
/**
 * @brief linked list of all buffers, through prev/next.
 * sorted by how recently the buffer was used.
 * head.next is most recent, while head.prev is least.
 */
LIST_HEAD(blkbuffercache_head);

void buffer_init()
{
	initlock(&bufcache_lock, "bufcache");
	for (struct blkbuf *bb = blkbufcache; bb < blkbufcache + NBUF; bb++) {
		// todo: initsleeplock
		list_add_front(&bb->node, &blkbuffercache_head);
	}
}