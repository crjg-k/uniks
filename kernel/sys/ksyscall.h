#ifndef __KERNEL_SYS_KSYSCALL_H__
#define __KERNEL_SYS_KSYSCALL_H__


#include <process/proc.h>
#include <uniks/defs.h>

/**
 * syscall convention:	a7=>syscall number
 * 			args=>a0~a5
 * 			retval=>a0
 */

// below header file has Linux syscall number
#if 0
	#include <asm/unistd_32.h>
#endif

#define SYS_exit    (1)
#define SYS_fork    (2)
#define SYS_read    (3)
#define SYS_write   (4)
#define SYS_open    (5)
#define SYS_close   (6)
#define SYS_waitpid (7)
#define SYS_creat   (8)
#define SYS_link    (9)
#define SYS_unlink  (10)
#define SYS_execve  (11)
#define SYS_lseek   (19)
#define SYS_getpid  (20)
#define SYS_kill    (37)
#define SYS_mkdir   (39)
#define SYS_rmdir   (40)
#define SYS_dup	    (41)
#define SYS_pipe    (42)
#define SYS_brk	    (45)
#define SYS_chroot  (61)
#define SYS_dup2    (63)
#define SYS_getppid (64)
#define SYS_readdir (89)
#define SYS_mmap    (90)
#define SYS_munmap  (91)
#define SYS_yield   (158)
#define SYS_msleep  (162)
#define SYS_getcwd  (183)


void syscall();
uint64_t argufetch(struct proc_t *p, int32_t n);
int32_t argstrfetch(uintptr_t addr, char *buf, int32_t max);


#endif /* !__KERNEL_SYS_KSYSCALL_H__ */