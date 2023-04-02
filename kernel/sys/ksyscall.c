#include "ksyscall.h"
#include <defs.h>
#include <kassert.h>
#include <kstdio.h>
#include <log.h>
#include <process/proc.h>
#include <structdef.h>


/**
 * @brief fetch the syscall nth argument 
 *
 * @param n the nth argument
 * @return uint64_t
 */
uint64_t argufetch(int32_t n)
{
	assert(n <= 5);
	struct proc *p = myproc();
	return ((uint64_t *)((char *)p->tf +
			     offsetof(struct trapframe, a0)))[n];
}


// execute system call actually
extern uint64_t sys_fork();
extern uint64_t sys_exit();
extern uint64_t sys_wait();
extern uint64_t sys_pipe();
extern uint64_t sys_read();
extern uint64_t sys_kill();
extern uint64_t sys_exec();
extern uint64_t sys_fstat();
extern uint64_t sys_chdir();
extern uint64_t sys_dup();
extern uint64_t sys_getpid();
extern uint64_t sys_sbrk();
extern uint64_t sys_sleep();
extern uint64_t sys_uptime();
extern uint64_t sys_open();
extern uint64_t sys_write();
extern uint64_t sys_mknod();
extern uint64_t sys_unlink();
extern uint64_t sys_link();
extern uint64_t sys_mkdir();
extern uint64_t sys_close();

static uint64_t (*syscalls[])() = {
	[SYS_fork] sys_fork,   [SYS_exec] sys_exec,	[SYS_write] sys_write,
	[SYS_sleep] sys_sleep, [SYS_getpid] sys_getpid,
};

#define NUM_SYSCALLS ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall()
{
	struct proc *p = myproc();
	int64_t num = p->tf->a7;
	if (num >= 0 and num < NUM_SYSCALLS and syscalls[num]) {
		p->tf->a0 = syscalls[num]();
		return;
	}
	tracef("undefined syscall %d, pid = %d, pname = %s", num, p->pid,
	       p->name);
}