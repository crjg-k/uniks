#include <defs.h>
#include <driver/clock.h>
#include <process/proc.h>
#include <sys/ksyscall.h>


uint64_t sys_getpid(void)
{
	return myproc()->pid;
}

uint64_t sys_fork()
{
	return do_fork();
}

uint64_t sys_exec()
{
	return do_exec();
}

uint64_t sys_sleep()
{
	int64_t n = argufetch(0);
	uint64_t ticks0;

	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (killed(myproc())) {
			release(&tickslock);
			return -1;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}