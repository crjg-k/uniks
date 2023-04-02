#ifndef __KERNEL_SYNC_SPINLOCK_H__
#define __KERNEL_SYNC_SPINLOCK_H__

#include <defs.h>
#include <log.h>

// mutual exclusion lock
struct spinlock {
	uint32_t locked;

// for debugging:
#if defined(LOG_LEVEL_DEBUG)
	char *name;    // name of the lock
	int32_t cpu;   // which cpu holding the lock
#endif
};

void initlock(struct spinlock *, char *);
void do_acquire(struct spinlock *);
void do_release(struct spinlock *);
#if defined(LOG_LEVEL_DEBUG)
	#define acquire(lk) \
		({ \
			debugf("acquire %s: %s %d", \
			       ((struct spinlock *)lk)->name, __FILE__, \
			       __LINE__); \
			do_acquire(lk); \
		})
#else
	#define acquire(lk) \
		({ \
			debugf("acquire: %s %d", __FILE__, __LINE__); \
			do_acquire(lk); \
		})
#endif
#if defined(LOG_LEVEL_DEBUG)
	#define release(lk) \
		({ \
			debugf("release %s: %s %d", \
			       ((struct spinlock *)lk)->name, __FILE__, \
			       __LINE__); \
			do_release(lk); \
		})
#else
	#define release(lk) \
		({ \
			debugf("release: %s %d", __FILE__, __LINE__); \
			do_release(lk); \
		})
#endif

int64_t holding(struct spinlock *lk);

#endif /* !__KERNEL_SYNC_SPINLOCK_H__ */