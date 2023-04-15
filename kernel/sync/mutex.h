#ifndef __KERNEL_SYNC_MUTEX_H__
#define __KERNEL_SYNC_MUTEX_H__

#include "spinlock.h"
#include <uniks/defs.h>
#include <uniks/list.h>

// mutex is namely sleep lock: long-term locks for processes
struct mutex_t {
	struct spinlock_t spinlk;   // spinlock protecting this mutex
	uint32_t locked;
	struct list_node_t waiters;   // waiting queue

// for debugging:
#if defined(LOG_LEVEL_DEBUG)
	char *name;    // Name of lock.
	int32_t pid;   // which process holds the lock
#endif
};

void mutex_init(struct mutex_t *m, char *name);
void mutex_acquire(struct mutex_t *m);
void mutex_release(struct mutex_t *m);
int32_t mutex_holding(struct mutex_t *m);


#endif /* !__KERNEL_SYNC_MUTEX_H__ */