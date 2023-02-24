#ifndef __KERNEL_SYNC_SPINLOCK_H__
#define __KERNEL_SYNC_SPINLOCK_H__

#include <defs.h>

// mutual exclusion lock
struct spinlock {
	uint32_t locked;

	// for debugging:
#if defined(LOG_LEVEL_DEBUG)
	char *name;	// name of the lock
	int32_t cpu;	// which cpu holding the lock
#endif
};

void initlock(struct spinlock *, char *);
void acquire(struct spinlock *);
void release(struct spinlock *);

#endif /* !__KERNEL_SYNC_SPINLOCK_H__ */