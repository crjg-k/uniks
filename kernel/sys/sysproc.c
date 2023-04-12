#include <defs.h>
#include <driver/clock.h>
#include <kassert.h>
#include <process/proc.h>
#include <sys/ksyscall.h>

extern uint64_t jiffy;
extern volatile uint64_t ticks;

int64_t sys_getpid(void)
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
	int64_t ms = argufetch(0);
	uint64_t target_ticks = ms / jiffy;   // timeslice need sleep
	target_ticks = target_ticks > 0 ? target_ticks
					: 1;   // at least sleep 1 timeslice

	acquire(&tickslock);
	target_ticks += ticks;
	release(&tickslock);

	struct proc *p = myproc();
	acquire(&pcblock[p->pid]);
	/**
	 * @brief process can be block only when it hasn't been blocked!
	 */
	assert(list_empty(&p->block_list));
	p->ticks = target_ticks;
	p->state = TASK_BLOCK;
	release(&pcblock[p->pid]);

	// insert process into priority_queue which record the wake time


	sched();
	return 0;
}