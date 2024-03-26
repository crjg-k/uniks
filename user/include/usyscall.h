#ifndef __USER_INCLUDE_USYSCALL_H__
#define __USER_INCLUDE_USYSCALL_H__


/**
 * syscall convention:	a7=>syscall number
 * 			args=>a0~a5
 * 			retval=>a0
 */


// system call numbers
#define SYS_restart  (0)
#define SYS_exit     (1)
#define SYS_fork     (2)
#define SYS_read     (3)
#define SYS_write    (4)
#define SYS_open     (5)
#define SYS_close    (6)
#define SYS_waitpid  (7)
#define SYS_creat    (8)
#define SYS_link     (9)
#define SYS_unlink   (10)
#define SYS_execve   (11)
#define SYS_chdir    (12)
#define SYS_time     (13)
#define SYS_mknod    (14)
#define SYS_chmod    (15)
#define SYS_lseek    (19)
#define SYS_getpid   (20)
#define SYS_sync     (36)
#define SYS_kill     (37)
#define SYS_rename   (38)
#define SYS_mkdir    (39)
#define SYS_rmdir    (40)
#define SYS_dup	     (41)
#define SYS_pipe     (42)
#define SYS_brk	     (45)
#define SYS_dup2     (63)
#define SYS_getppid  (64)
#define SYS_symlink  (83)
#define SYS_readdir  (89)
#define SYS_mmap     (90)
#define SYS_munmap   (91)
#define SYS_truncate (92)
#define SYS_stat     (106)
#define SYS_yield    (158)
#define SYS_msleep   (162)
#define SYS_getcwd   (183)


#if (__ASSEMBLER__ == 0)

#include <udefs.h>

// system call
int fork();
int open(const char *pathname, int flags);
int close(int fd);
int read(int fd, char *buf, long count);
int write(int fd, char *buf, long count);
int getpid();
int msleep(int msec);
int wait(int *status);
int waitpid(int pid, int *status);
int execve(const char *path, char *argv[], char *envp[]);
void _exit(int status);
int dup(int oldfd);
int pipe(int pipefd[2]);
int chdir(const char* path);
uintptr_t brk(void *addr);

#endif


#endif /* !__USER_INCLUDE_USYSCALL_H__ */