#include "ksyscall.h"
#include <defs.h>
#include <kassert.h>
#include <kstdio.h>
#include <log.h>
#include <process/proc.h>
#include <structdef.h>


uint64_t argufetch(int32_t n)
{
	assert(n <= 5);
	struct proc *p = myproc();
	return ((uint64_t *)((char *)p->tf +
			     offsetof(struct trapframe, a0)))[n];
}


// execute system call actually
extern uint64_t sys_fork(struct proc *);
extern uint64_t sys_exit(struct proc *);
extern uint64_t sys_wait(struct proc *);
extern uint64_t sys_pipe(struct proc *);
extern uint64_t sys_read(struct proc *);
extern uint64_t sys_kill(struct proc *);
extern uint64_t sys_exec(struct proc *);
extern uint64_t sys_fstat(struct proc *);
extern uint64_t sys_chdir(struct proc *);
extern uint64_t sys_dup(struct proc *);
extern uint64_t sys_getpid(struct proc *);
extern uint64_t sys_sbrk(struct proc *);
extern uint64_t sys_sleep(struct proc *);
extern uint64_t sys_uptime(struct proc *);
extern uint64_t sys_open(struct proc *);
extern uint64_t sys_write(struct proc *);
extern uint64_t sys_mknod(struct proc *);
extern uint64_t sys_unlink(struct proc *);
extern uint64_t sys_link(struct proc *);
extern uint64_t sys_mkdir(struct proc *);
extern uint64_t sys_close(struct proc *);

static uint64_t (*syscalls[])(struct proc *) = {
	[SYS_fork] sys_fork,
	[SYS_exec] sys_exec,
	[SYS_write] sys_write,
};

#define NUM_SYSCALLS ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall()
{
	struct proc *p = myproc();
	int64_t num = p->tf->a7;
	if (num >= 0 and num < NUM_SYSCALLS and syscalls[num]) {
		p->tf->a0 = syscalls[num](p);
		return;
	}
	debugf("undefined syscall %d, pid = %d, pname = %s", num, p->pid,
	       p->name);
}