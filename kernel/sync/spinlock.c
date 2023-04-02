#include "spinlock.h"
#include <defs.h>
#include <kassert.h>
#include <platform/riscv.h>


void initlock(struct spinlock *lk, char *name)
{
	lk->locked = 0;
#if defined(LOG_LEVEL_DEBUG)
	lk->name = name;
	lk->cpu = r_mhartid();
#endif
}

int64_t holding(struct spinlock *lk)
{
	return (lk->locked and
#if defined(LOG_LEVEL_DEBUG)
		lk->cpu == r_mhartid()
#else
		1
#endif
	);
}

void do_acquire(struct spinlock *lk)
{
	// assert(!holding(lk));

	while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
		debugf("kkk");

	__sync_synchronize();

#if defined(LOG_LEVEL_DEBUG)
	lk->cpu = r_mhartid();
#endif
}

void do_release(struct spinlock *lk)
{
	// assert(holding(lk));
#if defined(LOG_LEVEL_DEBUG)
	lk->cpu = -1;
#endif
	__sync_synchronize();

	__sync_lock_release(&lk->locked);
}