#include <defs.h>
#include <process/proc.h>


uint64_t sys_fork(struct proc *p)
{
	return do_fork();
}

uint64_t sys_exec(struct proc *p)
{
	return do_exec();
}