#include <device/clock.h>
#include <loader/elfloader.h>
#include <mm/vm.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/list.h>
#include <uniks/priority_queue.h>


extern uint64_t jiffy;
extern volatile uint64_t ticks;
extern int64_t do_execve(struct proc_t *p, char *pathname, char *argv[],
			 char *envp[]);


int64_t sys_getpid()
{
	return myproc()->pid;
}

int64_t sys_fork()
{
	return do_fork();
}

int64_t sys_execve()
{
	struct proc_t *p = myproc();
	char *file = (char *)vaddr2paddr(p->mm->pagetable, argufetch(p, 0)),
	     **argv = (char **)vaddr2paddr(p->mm->pagetable, argufetch(p, 1)),
	     **envp = (char **)vaddr2paddr(p->mm->pagetable, argufetch(p, 2));
	file++;
	extern char fib[];
	return do_execve(p, fib, argv, envp);
}

int64_t sys_msleep()
{
	struct proc_t *p = myproc();

	int64_t ms = argufetch(p, 0);
	uint64_t target_ticks = ms / jiffy;    // timeslice need sleep
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
	struct pair_t temppair = {target_ticks, p->pid};
	release(&pcblock[p->pid]);

	priority_queue_push(&sleep_queue.pqm, &temppair);

	release(&sleep_queue.sleep_lock);

	assert(p->magic == UNIKS_MAGIC);
	sched();
	return 0;
}

int64_t sys_waitpid()
{
	struct proc_t *p = myproc();
	int32_t pid = argufetch(p, 0);
	assert(p->pid != pid);
	assert(p != pcbtable[0]);

	acquire(&pcblock[pid]);
	if (pcbtable[pid] == NULL) {
		release(&pcblock[pid]);
		return -1;
	}
	if (pcbtable[pid]->state == TASK_ZOMBIE) {
		pcbtable[pid]->state = TASK_UNUSED;
		// if the proc[pid] has exited, tackle and return immediately
		acquire(&pcblock[p->pid]);
		int64_t *status_addr = (int64_t *)argufetch(p, 1);
		// verify_area(status_addr, sizeof(pcbtable[pid]->exitstate));
		copyout(p->mm->pagetable, (void *)status_addr,
			(void *)&pcbtable[pid]->exitstate,
			sizeof(pcbtable[pid]->exitstate));
		release(&pcblock[p->pid]);
		release(&pcblock[pid]);
	} else { /**
		  * @brief the proc with pid didn't exit, then block self and
		  * push it into wait list.
		  */

		/**
		 * @brief when block this process which will wait for the
		 * process with pid to exit, there is a need to acquire more
		 * than 1 lock @ the same time, so we hold the order that get
		 * the lock of the opposite side first and then get own's lock.
		 * Now, there is no deadlock phenomenon between wakeup-function
		 * in proc.c:do_exit() and this block-function occurred, I'm
		 * not sure that if it's okay to avoid it, but I hope that's bug
		 * free.
		 */
		proc_block(p, &pcbtable[pid]->wait_list, TASK_BLOCK);
		release(&pcblock[pid]);

		assert(p->magic == UNIKS_MAGIC);
		sched();
	}
	/**
	 * @brief free all memorys that the process holds, including
	 * the kstack, the pagetable, physical memory page which is
	 * mapped by pagetable and etc.
	 */
	recycle_exitedproc();
	return 0;
}

// will never return since process had exited
void sys_exit()
{
	do_exit(argufetch(myproc(), 0));
}