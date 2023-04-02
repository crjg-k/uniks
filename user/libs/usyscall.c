#include <stdarg.h>
#include <ulib.h>
#include <usyscall.h>


static inline long syscall(long num, ...)
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

int write(char *buf, long len)
{
	return syscall(SYS_write, 0, buf, len);
}
