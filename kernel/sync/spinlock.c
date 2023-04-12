#include "spinlock.h"
#include <defs.h>
#include <kassert.h>
#include <platform/riscv.h>
#include <process/proc.h>


void initlock(struct spinlock *lk, char *name)
{
	lk->locked = lk->repeat = 0;
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

void push_off()
{
	mycpu()->preintstat = interrupt_get();
	interrupt_off();
}

void pop_off()
{
	interrupt_set(mycpu()->preintstat);
}

void do_acquire(struct spinlock *lk)
{
	/**
	 * @brief disable interrupts to avoid deadlock incurred by extern
	 * interrupt and this must layed at initiate
	 */
	push_off();

	if (!holding(lk)) {
		while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
			debugf("kkk");
		assert(lk->repeat == 0);
		lk->repeat = 1;
	} else
		lk->repeat++;
	__sync_synchronize();

#if defined(LOG_LEVEL_DEBUG)
	lk->cpu = r_mhartid();
#endif
}

void do_release(struct spinlock *lk)
{
	assert(holding(lk));
	if(lk->repeat>1){
		lk->repeat--;
		return;
	}
	assert(lk->repeat == 1);
	lk->repeat = 0;
#if defined(LOG_LEVEL_DEBUG)
	lk->cpu = -1;
#endif
	__sync_synchronize();

	__sync_lock_release(&lk->locked);
	pop_off();
}