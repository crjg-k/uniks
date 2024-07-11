#ifndef __KERNEL_SYS_KSYSCALL_H__
#define __KERNEL_SYS_KSYSCALL_H__


#include <process/proc.h>
#include <uniks/defs.h>

/**
 * syscall convention:	a7=>syscall number
 * 			args=>a0~a5
 * 			retval=>a0
 */

// below header file contains Linux syscall number
#if 0
	#include <asm/unistd_64.h>
#endif

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
#define SYS_kill     (62)
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


void syscall();
uint64_t argufetch(struct proc_t *p, int32_t n);
int32_t argstrfetch(uintptr_t addr, char *buf, int32_t max);


#endif /* !__KERNEL_SYS_KSYSCALL_H__ */