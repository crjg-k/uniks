#ifndef __KERNEL_SYNC_SPINLOCK_H__
#define __KERNEL_SYNC_SPINLOCK_H__

#include <uniks/defs.h>
#include <uniks/log.h>


// mutual exclusion lock, could reentry by same hart
struct spinlock_t {
	volatile uint32_t locked;

// for debugging:
#if defined(LOG_LEVEL_DEBUG)
	char *name;    // name of the lock
	int32_t cpu;   // which cpu holding the lock
#endif
};

void initlock(struct spinlock_t *lk, char *name);
int64_t holding(struct spinlock_t *lk);

void push_off();
void pop_off();

void do_acquire(struct spinlock_t *lk);
void do_release(struct spinlock_t *lk);

#if defined(LOG_LEVEL_DEBUG)
	#define acquire(lk) \
		({ \
			debugf("acquire %s: %s %d", \
			       ((struct spinlock_t *)lk)->name, __FILE__, \
			       __LINE__); \
			do_acquire(lk); \
		})
#else
	#define acquire(lk) ({ do_acquire(lk); })
#endif
#if defined(LOG_LEVEL_DEBUG)
	#define release(lk) \
		({ \
			debugf("release %s: %s %d", \
			       ((struct spinlock_t *)lk)->name, __FILE__, \
			       __LINE__); \
			do_release(lk); \
		})
#else
	#define release(lk) ({ do_release(lk); })
#endif


#endif /* !__KERNEL_SYNC_SPINLOCK_H__ */