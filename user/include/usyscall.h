#ifndef __USER_INCLUDE_USYSCALL_H__
#define __USER_INCLUDE_USYSCALL_H__


/**
 * syscall convention:	a7=>syscall number
 * 			args=>a0~a5
 * 			retval=>a0
 */


// system call numbers
#define SYS_read     (0)
#define SYS_write    (1)
#define SYS_open     (2)
#define SYS_close    (3)
#define SYS_stat     (4)
#define SYS_fstat    (5)
#define SYS_lstat    (6)
#define SYS_lseek    (8)
#define SYS_mmap     (9)
#define SYS_munmap   (11)
#define SYS_brk	     (12)
#define SYS_pipe     (22)
#define SYS_yield    (24)
#define SYS_dup	     (32)
#define SYS_dup2     (33)
#define SYS_msleep   (35)
#define SYS_getpid   (39)
#define SYS_shutdown (48)
#define SYS_fork     (57)
#define SYS_execve   (59)
#define SYS_exit     (60)
#define SYS_wait4    (61)
#define SYS_fcntl    (72)
#define SYS_truncate (76)
#define SYS_getdents (78)
#define SYS_getcwd   (79)
#define SYS_chdir    (80)
#define SYS_rename   (82)
#define SYS_mkdir    (83)
#define SYS_rmdir    (84)
#define SYS_creat    (85)
#define SYS_link     (86)
#define SYS_unlink   (87)
#define SYS_symlink  (88)
#define SYS_readlink (89)
#define SYS_chmod    (90)
#define SYS_getppid  (110)
#define SYS_sync     (162)


#if (__ASSEMBLER__ == 0)

	#include <udefs.h>
	#include <ustat.h>
	#include <udirent.h>

// system call
pid_t fork();
int open(const char *pathname, int flags);
int close(int fd);
int read(int fd, char *buf, size_t count);
int write(int fd, const char *buf, size_t count);
pid_t getpid();
int msleep(int msec);
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status);
int execve(const char *path, char *argv[], char *envp[]);
void _exit(int status);
int dup(int oldfd);
int pipe(int pipefd[2]);
int chdir(const char *path);
uintptr_t brk(void *addr);
long getdents(int fd, struct dirent *dirp, size_t count);
int stat(const char *pathname, struct stat *statbuf);
int fstat(int fd, struct stat *statbuf);
int lstat(const char *pathname, struct stat *statbuf);
char *getcwd(char *buf, size_t size);

#endif


#endif /* !__USER_INCLUDE_USYSCALL_H__ */