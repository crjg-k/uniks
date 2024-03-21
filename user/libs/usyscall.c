#include <stdarg.h>
#include <udefs.h>
#include <usyscall.h>


static long syscall(long num, ...)
{
	va_list ap;
	va_start(ap, num);
	register long a0 asm("a0") = va_arg(ap, long);
	register long a1 asm("a1") = va_arg(ap, long);
	register long a2 asm("a2") = va_arg(ap, long);
	register long a3 asm("a3") = va_arg(ap, long);
	register long a4 asm("a4") = va_arg(ap, long);
	register long a5 asm("a5") = va_arg(ap, long);
	register long a7 asm("a7") = num;
	va_end(ap);

	asm volatile("ecall"
		     : "+r"(a0)
		     : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
		       "r"(a7)
		     : "memory");
	return a0;
}


int fork()
{
	return syscall(SYS_fork);
}

int getpid()
{
	return syscall(SYS_getpid);
}

int open(const char *pathname, int flags)
{
	return syscall(SYS_open, pathname, flags);
}

int read(int fd, char *buf, long count)
{
	return syscall(SYS_read, fd, buf, count);
}

int write(int fd, char *buf, long count)
{
	return syscall(SYS_write, fd, buf, count);
}

int msleep(int msec)
{
	return syscall(SYS_msleep, msec);
}

int waitpid(int pid, int *status)
{
	return syscall(SYS_waitpid, pid, status);
}

int wait(int *status)
{
	return waitpid(-1, status);
}

int execve(const char *path, char *argv[], char *envp[])
{
	return syscall(SYS_execve, path, argv, envp);
}

void _exit(int status)
{
	syscall(SYS_exit, status);
}

int dup(int oldfd)
{
	return syscall(SYS_dup, oldfd);
}