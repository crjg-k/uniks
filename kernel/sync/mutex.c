#include "mutex.h"
#include <process/proc.h>
#include <uniks/kassert.h>


void mutex_init(struct mutex_t *m, char *name)
{
	m->locked = 0;
	INIT_LIST_HEAD(&m->waiters);
	m->name = name;
	m->pid = 0;
}

int32_t mutex_holding(struct mutex_t *m)
{
	return (m->locked and (m->pid == myproc()->pid));
}

void mutex_acquire(struct mutex_t *m)
{
	struct proc_t *p = myproc();
	while (__sync_lock_test_and_set(&m->locked, 1) != 0) {
		acquire(&pcblock[p->pid]);
		list_add_front(&p->block_list, &m->waiters);
		p->state = TASK_BLOCK;
		sched();
		release(&pcblock[p->pid]);
	}
	__sync_synchronize();
	m->pid = myproc()->pid;
}

void mutex_release(struct mutex_t *m)
{
	assert(mutex_holding(m));   // ensure that this mutex had been held

	m->pid = 0;
	__sync_synchronize();
	__sync_lock_release(&m->locked);
	proc_unblock_all(&m->waiters);
	assert(list_empty(&m->waiters));
}