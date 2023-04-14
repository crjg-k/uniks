#include <defs.h>
#include <driver/clock.h>
#include <kassert.h>
#include <list.h>
#include <priority_queue.h>
#include <process/proc.h>
#include <sys/ksyscall.h>

extern uint64_t jiffy;
extern volatile uint64_t ticks;
extern struct sleep_queue_t sleep_queue;
extern int32_t copyout(pagetable_t pagetable, uintptr_t dstva, char *src,
		       uint64_t len);


int64_t sys_getpid()
{
	return myproc()->pid;
}

int64_t sys_fork()
{
	return do_fork();
}

int64_t sys_exec()
{
	return do_exec();
}

int64_t sys_msleep()
{
	struct proc *p = myproc();

	int64_t ms = argufetch(p, 0);
	uint64_t target_ticks = ms / jiffy;   // timeslice need sleep
	target_ticks = target_ticks > 0 ? target_ticks
					: 1;   // at least sleep 1 timeslice

	acquire(&tickslock);
	target_ticks += ticks;
	release(&tickslock);

	acquire(&sleep_queue.sleep_lock);
	acquire(&pcblock[p->pid]);
	// hint: process can be blocked only when it hasn't been blocked!
	assert(list_empty(&p->block_list));
	p->state = TASK_BLOCK;

	// push process into priority_queue which record the time to wakeup
	struct pair temppair = {target_ticks, p->pid};
	release(&pcblock[p->pid]);

	priority_queue_push(&sleep_queue.pqm, &temppair);

	release(&sleep_queue.sleep_lock);

	assert(p->magic == UNIKS_MAGIC);
	sched();
	return 0;
}

void sys_waitpid()
{
	struct proc *p = myproc();
	int32_t pid = argufetch(p, 0);
	assert(p->pid != pid);
	assert(p != pcbtable[0]);

	if (pcbtable[pid] != NULL and pcbtable[pid]->state == TASK_ZOMBIE) {
		// if the proc[pid] has exited, tackle and return immediately
		acquire(&pcblock[p->pid]);
		int64_t *status_addr = (int64_t *)argufetch(p, 1);
		copyout(p->pagetable, (uintptr_t)status_addr,
			(char *)&pcbtable[pid]->exitstate,
			sizeof(pcbtable[pid]->exitstate));
		release(&pcblock[p->pid]);
		/**
		 * @brief free all memorys that the process holds, including
		 * the kstack, the pagetable, physical memory page which is
		 * mapped by pagetable and etc.
		 */

	} else {   // the proc with pid didn't exit, then push it into wait list
		/**
		 * @brief there are no need to acquire own process lock
		 */
		acquire(&pcblock[pid]);
		p->state = TASK_BLOCK;
		list_add_front(&p->block_list, &pcbtable[pid]->wait_list);
		release(&pcblock[pid]);
		sched();
	}
}

void sys_exit()
{
	do_exit(argufetch(myproc(), 0));
}