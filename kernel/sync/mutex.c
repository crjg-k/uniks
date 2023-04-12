#include "mutex.h"
#include <kassert.h>
#include <process/proc.h>


void mutex_init(struct mutex *m, char *name)
{
	initlock(&m->spinlk, "mutex");
	m->locked = 0;
	INIT_LIST_HEAD(&m->waiters);
#if defined(LOG_LEVEL_DEBUG)
	m->name = name;
	m->pid = -1;
#endif
}

int32_t mutex_holding(struct mutex *m)
{
	int32_t res;

	acquire(&m->spinlk);
	res = (m->locked and
#if defined(LOG_LEVEL_DEBUG)
	       (m->pid == myproc()->pid)
#else
	       1
#endif
	);
	release(&m->spinlk);
	return res;
}

void mutex_acquire(struct mutex *m)
{
	acquire(&m->spinlk);
	while (mutex_holding(m)) {
		// this mutex had been held by others, then block
		proc_block(myproc(), &m->waiters, TASK_BLOCK);
	}
	assert(!mutex_holding(m));   // no process hold this mutex

	m->locked = 1;
#if defined(LOG_LEVEL_DEBUG)
	m->pid = myproc()->pid;
#endif
	release(&m->spinlk);
}

void mutex_release(struct mutex *m)
{
	acquire(&m->spinlk);
	assert(mutex_holding(m));   // ensure that this mutex had been held
	m->locked = 0;
#if defined(LOG_LEVEL_DEBUG)
	m->pid = 0;
#endif
	if (!list_empty(&m->waiters)) {
		// waiting queue has item(s), need to wakeup other processes
		/**
		 * @brief search from back to front to ensure fair: the last is
		 * the first comed
		 */
		struct proc *p = element_entry(list_prev(&m->waiters),
					       struct proc, block_list);
		assert(p->magic == UNIKS_MAGIC);
		proc_unblock(p);
		release(&m->spinlk);
		yield();   // give other process a chance to acquire this lock
	} else
		release(&m->spinlk);
}