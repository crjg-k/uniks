#include "blkbuffer.h"
#include <uniks/param.h>


struct spinlock_t bufcache_lock;
struct blkbuf_t blkbufcache[NBUF];
/**
 * @brief linked list of all buffers, through prev/next.
 * sorted by how recently the buffer was used.
 * head.next is most recent, while head.prev is least.
 */
LIST_HEAD(blkbuffercache_head);

void buffer_init()
{
	initlock(&bufcache_lock, "bufcache");
	for (struct blkbuf_t *bb = blkbufcache; bb < blkbufcache + NBUF; bb++) {
		// todo: init mutex lock
		list_add_front(&bb->node, &blkbuffercache_head);
	}
}