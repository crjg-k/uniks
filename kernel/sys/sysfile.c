#include <defs.h>
#include <driver/console.h>
#include <process/proc.h>
#include <sys/ksyscall.h>

char dst[32];

extern int64_t copyin(pagetable_t pagetable, char *dst, uint64_t srcva,
		      uint64_t len);

uint64_t fileswrite(int32_t n)
{
	for (int32_t i = 0; i < n; i++)
		console_putchar(dst[i]);
	return n;
}

uint64_t sys_write(struct proc *p)
{
	// hint: now, temporary output to console directly
	int64_t n = argufetch(2);
	copyin(p->pagetable, dst, argufetch(1), n);
	return fileswrite(n);
}