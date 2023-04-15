#include <driver/console.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>

char dst[32];

extern int64_t copyin(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t len);

uint64_t fileswrite(int32_t n)
{
	for (int32_t i = 0; i < n; i++)
		console_putchar(dst[i]);
	return n;
}

uint64_t sys_write(struct proc_t *p)
{
	// hint: now, temporary output to console directly
	int64_t n = argufetch(p, 2);
	copyin(p->pagetable, dst, (uintptr_t)argufetch(p, 1), n);
	return fileswrite(n);
}
