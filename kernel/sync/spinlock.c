#include "spinlock.h"
#include <platform/riscv.h>
#include <process/proc.h>
#include <uniks/defs.h>
#include <uniks/kassert.h>


void initlock(struct spinlock_t *lk, char *name)
{
	lk->locked = 0;
#if defined(LOG_LEVEL_DEBUG)
	lk->name = name;
	lk->cpu = r_mhartid();
#endif
}

int64_t holding(struct spinlock_t *lk)
{
	return (lk->locked and
#if defined(LOG_LEVEL_DEBUG)
		lk->cpu == r_mhartid()
#else
		1
#endif
	);
}

/**
 * @brief push_off/pop_off are like intr_off()/intr_on() except that they are
 * matched: it takes two pop_off()s to undo two push_off()s.  Also, if
 * interrupts are initially off, then push_off, pop_off leaves them off.
 */
void push_off()
{
	uint64_t old = interrupt_get();

	interrupt_off();
	struct cpu_t *c = mycpu();
	if (c->repeat == 0)
		c->preintstat = old;
	c->repeat++;
}
void pop_off()
{
	struct cpu_t *c = mycpu();
	assert(!(interrupt_get() & SSTATUS_SIE));
	assert(c->repeat >= 1);
	c->repeat--;
	if (c->repeat == 0 and (c->preintstat & SSTATUS_SIE))
		interrupt_on();
}

void do_acquire(struct spinlock_t *lk)
{
	/**
	 * @brief disable interrupts to avoid deadlock incurred by extern
	 * interrupt and this must layed at initiate
	 */
	push_off();

	/**
	 * @brief avoid obtaining duplicate locks on the same CPU itself
	 */
	assert(!holding(lk));

	while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
		debugf("kkk");

	__sync_synchronize();

#if defined(LOG_LEVEL_DEBUG)
	lk->cpu = r_mhartid();
#endif
}

void do_release(struct spinlock_t *lk)
{
	assert(holding(lk));

#if defined(LOG_LEVEL_DEBUG)
	lk->cpu = -1;
#endif
	__sync_synchronize();

	__sync_lock_release(&lk->locked);
	pop_off();
}