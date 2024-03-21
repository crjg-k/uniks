#include <device/clock.h>
#include <file/file.h>
#include <loader/elfloader.h>
#include <mm/vm.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/errno.h>
#include <uniks/kassert.h>
#include <uniks/list.h>
#include <uniks/priority_queue.h>


extern int64_t do_execve(struct proc_t *p, char *path, char *argv[],
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
	int32_t res = -EFAULT;
	struct proc_t *p = myproc();
	char *path = kmalloc(MAX_PATH_LEN);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, MAX_PATH_LEN) < 0)
		goto ret;

	char **argv = (char **)argufetch(p, 1),
	     **envp = (char **)argufetch(p, 2);
	res = do_execve(p, path, argv, envp);

ret:
	kfree(path);
	return res;
}

int64_t sys_msleep()
{
	struct proc_t *p = myproc();
	uint64_t ms = argufetch(p, 0);
	uint64_t target_ticks = ms / jiffy;   // timeslice need sleep
	if (target_ticks < 1)		      // at least sleep 1 timeslice
		target_ticks = 1;

	acquire(&sleep_queue.sleep_lock);
	// push process into priority_queue which record the time to wakeup
	struct pair_t temppair = {target_ticks + ticks, p->pid};
	priority_queue_push(&sleep_queue.pqm, &temppair);
	release(&sleep_queue.sleep_lock);

	acquire(&pcblock[p->pid]);
	p->state = TASK_BLOCK;
	sched();
	release(&pcblock[p->pid]);

	return 0;
}

int64_t sys_waitpid()
{
	struct proc_t *p = myproc(), *target_p;
	pid_t target_pid = argufetch(p, 0);
	assert(p != FIRST_PROC);
	if (p->pid == target_pid)
		return -1;
	uintptr_t status_vaddr = argufetch(p, 1);

	if (target_pid == -1) {
		// means that wait for any childproc to exit
		int32_t i;
		struct proc_t *pp;
repeat:
		for (i = 1; i < NPROC; i++) {
			if (i == p->pid)
				continue;
			acquire(&pcblock[i]);
			if ((pp = pcbtable[i]) == NULL) {
				release(&pcblock[i]);
				continue;
			}
			if (pp->parentpid == p->pid and
			    pp->state == TASK_ZOMBIE) {
				break;
			}
			release(&pcblock[i]);
		}
		if (i != NPROC) {   // found one exited childproc
			// hint: There might be no concurrent bugs
			p->w4child = 0;

			target_pid = pp->pid;
			target_p = pcbtable[target_pid];
		} else {
			acquire(&pcblock[p->pid]);
			p->w4child = 1;
			p->state = TASK_BLOCK;
			sched();
			release(&pcblock[p->pid]);
			goto repeat;
		}
	} else if (target_pid > 0) {
		// means that wait for the specified childproc to exit
		target_p = pcbtable[target_pid];
		acquire(&pcblock[target_pid]);
		if (target_p == NULL) {
			release(&pcblock[target_pid]);
			return -1;
		}
		while (target_p->state != TASK_ZOMBIE) {
			/**
			 * @brief the proc with pid didn't exit, then block self
			 * and push it into wait list.
			 */
			proc_block(&target_p->wait_list, &pcblock[target_pid]);
		}
	} else
		BUG();

	assert(target_p->state == TASK_ZOMBIE);
	uint64_t len = sizeof(target_p->exitstate);
	if (status_vaddr != 0) {
		verify_area(p->mm, status_vaddr, len, PTE_W | PTE_U);
		assert(copyout(p->mm->pagetable, (void *)status_vaddr,
			       (void *)&(target_p->exitstate), len) != -1);
	}

	freepid(target_p->pid);
	pages_free(target_p->tf);
	pcbtable[target_pid] = NULL;
	release(&pcblock[target_pid]);

	return target_pid;
}

// will never return since process had exited
void sys_exit()
{
	do_exit(argufetch(myproc(), 0));
}