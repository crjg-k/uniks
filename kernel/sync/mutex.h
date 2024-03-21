#ifndef __KERNEL_SYNC_MUTEX_H__
#define __KERNEL_SYNC_MUTEX_H__


#include "spinlock.h"
#include <uniks/defs.h>
#include <uniks/list.h>

// mutex is namely sleep lock: long-term locks for processes
struct mutex_t {
	volatile uint32_t locked;

	struct spinlock_t lk;	      // spinlock protecting below wait_list
	struct list_node_t waiters;   // waiting queue

	// for debugging:
	char *name;   // Name of lock.
	pid_t pid;    // which process holds the lock
};

void mutex_init(struct mutex_t *m, char *name);
int32_t mutex_holding(struct mutex_t *m);
void mutex_acquire(struct mutex_t *m);
void mutex_release(struct mutex_t *m);


#endif /* !__KERNEL_SYNC_MUTEX_H__ */