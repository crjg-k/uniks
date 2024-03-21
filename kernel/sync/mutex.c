#include "mutex.h"
#include <process/proc.h>
#include <uniks/kassert.h>


void mutex_init(struct mutex_t *m, char *name)
{
	m->locked = 0;
	initlock(&m->lk, "mtxlk");
	INIT_LIST_HEAD(&m->waiters);
	m->name = name;
	m->pid = 0;
}

int32_t mutex_holding(struct mutex_t *m)
{
	int32_t res;
	acquire(&m->lk);
	res = (m->locked and (m->pid == myproc()->pid));
	release(&m->lk);
	return res;
}

void mutex_acquire(struct mutex_t *m)
{
	acquire(&m->lk);
	while (m->locked) {
		proc_block(&m->waiters, &m->lk);
	}
	m->locked = 1;
	m->pid = myproc()->pid;
	release(&m->lk);
}

void mutex_release(struct mutex_t *m)
{
	assert(mutex_holding(m));   // ensure that this mutex had been held

	acquire(&m->lk);
	m->pid = m->locked = 0;
	proc_unblock_all(&m->waiters);
	assert(list_empty(&m->waiters));
	release(&m->lk);
}