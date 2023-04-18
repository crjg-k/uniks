#ifndef __USER_INCLUDE_USYSCALL_H__
#define __USER_INCLUDE_USYSCALL_H__


/**
 * syscall convention:	a7=>syscall number
 * 			args=>a0~a5
 * 			retval=>a0
 */


// system call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_waitpid 3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup	    10
#define SYS_getpid  11
#define SYS_sbrk    12
#define SYS_msleep  13
#define SYS_uptime  14
#define SYS_open    15
#define SYS_write   16
#define SYS_mknod   17
#define SYS_unlink  18
#define SYS_link    19
#define SYS_mkdir   20
#define SYS_close   21


#if (__ASSEMBLER__ == 0)

// system call
int fork();
int read(char *buf, long count);
int write(char *buf, long count);
int getpid();
int msleep(int msec);
int waitpid(int pid, int *status);
void exit(int status);

#endif


#endif /* !__USER_INCLUDE_USYSCALL_H__ */