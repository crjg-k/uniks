#ifndef __KERNEL_SYNC_MUTEX_H__
#define __KERNEL_SYNC_MUTEX_H__

#include "spinlock.h"
#include <defs.h>
#include <list.h>

/**
 * @brief mutex is namely sleep lock: long-term locks for processes
 */
struct mutex {
	struct spinlock spinlk;	  // spinlock protecting this mutex
	uint32_t locked;
	struct list_node waiters;   // waiting queue

// for debugging:
#if defined(LOG_LEVEL_DEBUG)
	char *name;    // Name of lock.
	int32_t pid;   // which process holds the lock
#endif
};

void mutex_init(struct mutex *m, char *name);
void mutex_acquire(struct mutex *m);
void mutex_release(struct mutex *m);
int32_t mutex_holding(struct mutex *m);


#endif /* !__KERNEL_SYNC_MUTEX_H__ */