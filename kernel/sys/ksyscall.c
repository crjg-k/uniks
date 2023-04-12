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
extern int64_t sys_fork();
extern int64_t sys_exit();
extern int64_t sys_wait();
extern int64_t sys_pipe();
extern int64_t sys_read();
extern int64_t sys_kill();
extern int64_t sys_exec();
extern int64_t sys_fstat();
extern int64_t sys_chdir();
extern int64_t sys_dup();
extern int64_t sys_getpid();
extern int64_t sys_sbrk();
extern int64_t sys_msleep();
extern int64_t sys_uptime();
extern int64_t sys_open();
extern int64_t sys_write();
extern int64_t sys_mknod();
extern int64_t sys_unlink();
extern int64_t sys_link();
extern int64_t sys_mkdir();
extern int64_t sys_close();

static int64_t (*syscalls[])() = {
	[SYS_fork] sys_fork,	 [SYS_exec] sys_exec,	  [SYS_write] sys_write,
	[SYS_msleep] sys_msleep, [SYS_getpid] sys_getpid,
};

#define NUM_SYSCALLS ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall()
{
	struct proc *p = myproc();
	int64_t num = p->tf->a7;
	if (num >= 0 and num < NUM_SYSCALLS and syscalls[num]) {
		p->tf->a0 = syscalls[num]();
		assert(myproc()->magic == UNIKS_MAGIC);
		return;
	}
	tracef("undefined syscall %d, pid = %d, pname = %s", num, p->pid,
	       p->name);
}