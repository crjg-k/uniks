#include <stdarg.h>
#include <ufcntl.h>
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


pid_t fork()
{
	return syscall(SYS_fork);
}

pid_t getpid()
{
	return syscall(SYS_getpid);
}

pid_t getppid()
{
	return syscall(SYS_getppid);
}

int open(const char *pathname, int flags, ...)
{
	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		return syscall(SYS_open, pathname, flags, va_arg(ap, int));
		va_end(ap);
	} else
		return syscall(SYS_open, pathname, flags);
}

int creat(const char *pathname, mode_t mode)
{
	return syscall(SYS_creat, pathname, mode);
}

int truncate(const char *path, size_t length)
{
	return syscall(SYS_truncate, path, length);
}

int close(int fd)
{
	return syscall(SYS_close, fd);
}

int read(int fd, char *buf, size_t count)
{
	return syscall(SYS_read, fd, buf, count);
}

int write(int fd, const char *buf, size_t count)
{
	return syscall(SYS_write, fd, buf, count);
}

void sync()
{
	syscall(SYS_sync);
}

void shutdown()
{
	syscall(SYS_shutdown);
}

int msleep(int msec)
{
	return syscall(SYS_msleep, msec);
}

pid_t waitpid(pid_t pid, int *status)
{
	return syscall(SYS_wait4, pid, status);
}

pid_t wait(int *status)
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

int pipe(int pipefd[2])
{
	return syscall(SYS_pipe, pipefd);
}

int mkdir(const char *pathname, mode_t mode)
{
	return syscall(SYS_mkdir, pathname, mode);
}

int link(const char *oldpath, const char *newpath)
{
	return syscall(SYS_link, oldpath, newpath);
}

int symlink(const char *target, const char *linkpath)
{
	return syscall(SYS_symlink, target, linkpath);
}

int chdir(const char *path)
{
	return syscall(SYS_chdir, path);
}

int chmod(const char *pathname, mode_t mode)
{
	return syscall(SYS_chmod, pathname, mode);
}

uintptr_t brk(void *addr)
{
	return syscall(SYS_brk, addr);
}

long getdents(int fd, struct dirent *dirp, size_t count)
{
	return syscall(SYS_getdents, fd, dirp, count);
}

int stat(const char *pathname, struct stat *statbuf)
{
	return syscall(SYS_stat, pathname, statbuf);
}

int fstat(int fd, struct stat *statbuf)
{
	return syscall(SYS_fstat, fd, statbuf);
}

int lstat(const char *pathname, struct stat *statbuf)
{
	return syscall(SYS_lstat, pathname, statbuf);
}

char *getcwd(char *buf, size_t size)
{
	return (char *)syscall(SYS_getcwd, buf, size);
}
