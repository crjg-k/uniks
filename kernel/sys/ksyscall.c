#include "ksyscall.h"
#include <mm/vm.h>
#include <process/proc.h>
#include <uniks/kassert.h>
#include <uniks/kstdio.h>
#include <uniks/list.h>
#include <uniks/log.h>


/**
 * @brief fetch the syscall nth argument of process p
 *
 * @param p target process pointer
 * @param n the nth argument
 * @return uint64_t
 */
uint64_t argufetch(struct proc_t *p, int32_t n)
{
	assert(n <= 5);
	return ((uint64_t *)((char *)p->tf +
			     offsetof(struct trapframe_t, a0)))[n];
}

/**
 * @brief Fetch the nth word-sized system call argument as a null-terminated
 * string. Copies into buf, at most max.
 * @param vaddr
 * @param buf
 * @param max
 * @return int32_t: `strlen(buf)` if OK (excluding '\0'), -1 if error.
 */
int32_t argstrfetch(uintptr_t vaddr, char *buf, int32_t max)
{
	struct mm_struct *mm = myproc()->mm;
	int32_t got_null = -1, len = 0, offset = OFFSETPAGE(vaddr), bytes;

	while (got_null == -1) {
		bytes = PGSIZE - offset;
		if (bytes > max)
			bytes = max;
		if (verify_area(mm, vaddr, bytes, PTE_R | PTE_U) < 0)
			return -1;
		got_null =
			copyin_string(mm->pagetable, buf + len, vaddr, bytes);

		if (got_null > 0)
			len += got_null;
		offset = 0;
		max -= bytes;
		vaddr += bytes;
	}

	return len;
}


// execute system call actually

extern int64_t sys_fork();
extern int64_t sys_exit();
extern int64_t sys_wait4();
extern int64_t sys_pipe();
extern int64_t sys_read();
extern int64_t sys_kill();
extern int64_t sys_execve();
extern int64_t sys_stat();
extern int64_t sys_fstat();
extern int64_t sys_chdir();
extern int64_t sys_dup();
extern int64_t sys_getpid();
extern int64_t sys_getppid();
extern int64_t sys_msleep();
extern int64_t sys_open();
extern int64_t sys_write();
extern int64_t sys_link();
extern int64_t sys_unlink();
extern int64_t sys_mkdir();
extern int64_t sys_close();
extern int64_t sys_brk();
extern int64_t sys_getcwd();
extern int64_t sys_getdents();
extern int64_t sys_lseek();
extern int64_t sys_truncate();
extern int64_t sys_rename();
extern int64_t sys_rmdir();
extern int64_t sys_creat();
extern int64_t sys_symlink();
extern int64_t sys_readlink();
extern int64_t sys_chmod();
extern int64_t sys_sync();
extern int64_t sys_shutdown();

static int64_t (*syscalls[])() = {
	[SYS_fork] sys_fork,	 [SYS_execve] sys_execve,
	[SYS_write] sys_write,	 [SYS_msleep] sys_msleep,
	[SYS_getpid] sys_getpid, [SYS_exit] sys_exit,
	[SYS_wait4] sys_wait4,	 [SYS_read] sys_read,
	[SYS_open] sys_open,	 [SYS_dup] sys_dup,
	[SYS_close] sys_close,	 [SYS_pipe] sys_pipe,
	[SYS_fstat] sys_fstat,	 [SYS_getdents] sys_getdents,
	[SYS_chdir] sys_chdir,	 [SYS_brk] sys_brk,
	[SYS_getcwd] sys_getcwd, [SYS_stat] sys_stat,
	[SYS_kill] sys_kill,	 [SYS_getppid] sys_getppid,
	[SYS_lseek] sys_lseek,	 [SYS_mkdir] sys_mkdir,
	[SYS_sync] sys_sync,	 [SYS_shutdown] sys_shutdown,
	[SYS_creat] sys_creat,	 [SYS_truncate] sys_truncate,
	[SYS_chmod] sys_chmod,	 [SYS_unlink] sys_unlink,
	[SYS_link] sys_link,	 [SYS_rmdir] sys_rmdir,
};

#define NUM_SYSCALLS ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall()
{
	struct proc_t *p = myproc();
	int64_t num = p->tf->a7;
	if (num >= 0 and num < NUM_SYSCALLS and syscalls[num]) {
		p->tf->a0 = syscalls[num]();
		assert(p->magic == UNIKS_MAGIC);
		return;
	}
	tracef("undefined syscall %d, pid = %d, pname = %s", num, p->pid,
	       p->name);
}