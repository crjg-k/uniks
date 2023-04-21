#include "mutex.h"
#include <process/proc.h>
#include <uniks/kassert.h>


void mutex_init(struct mutex_t *m, char *name)
{
	initlock(&m->spinlk, "mutex");
	m->locked = 0;
	INIT_LIST_HEAD(&m->waiters);
#if defined(LOG_LEVEL_DEBUG)
	m->name = name;
	m->pid = -1;
#endif
}

int32_t mutex_holding(struct mutex_t *m)
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

void mutex_acquire(struct mutex_t *m)
{
	acquire(&m->spinlk);
	while (mutex_holding(m)) {
		// this mutex had been held by others, then block
		proc_block(myproc(), &m->waiters, TASK_BLOCK);
		release(&m->spinlk);
		sched();
		acquire(&m->spinlk);
	}
	assert(!mutex_holding(m));   // no process hold this mutex

	m->locked = 1;
#if defined(LOG_LEVEL_DEBUG)
	m->pid = myproc()->pid;
#endif
	release(&m->spinlk);
}

void mutex_release(struct mutex_t *m)
{
	acquire(&m->spinlk);
	assert(mutex_holding(m));   // ensure that this mutex had been held
	m->locked = 0;
#if defined(LOG_LEVEL_DEBUG)
	m->pid = -1;
#endif
	while (!list_empty(&m->waiters)) {
		// waiting queue has item(s), need to wakeup other processes
		struct proc_t *p = proc_unblock(&m->waiters);

		release(&pcblock[p->pid]);
	}
	assert(list_empty(&m->waiters));

	release(&m->spinlk);
	// give other processes a chance to acquire this lock
	yield();
}