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
	while (mutex_holding(m)) {
		acquire(&m->spinlk);
		// this mutex had been held by others, then block
		proc_block(myproc(), &m->waiters, TASK_BLOCK);
		release(&m->spinlk);
		sched();
	}
	assert(!mutex_holding(m));   // no process hold this mutex

	acquire(&m->spinlk);
	m->locked = 1;
#if defined(LOG_LEVEL_DEBUG)
	m->pid = myproc()->pid;
#endif
	release(&m->spinlk);
}

void mutex_release(struct mutex_t *m)
{
	assert(mutex_holding(m));   // ensure that this mutex had been held

	acquire(&m->spinlk);
	m->locked = 0;
#if defined(LOG_LEVEL_DEBUG)
	m->pid = -1;
#endif
	proc_unblock_all(&m->waiters);
	assert(list_empty(&m->waiters));

	release(&m->spinlk);
	// give other processes a chance to acquire this lock
	yield();
}