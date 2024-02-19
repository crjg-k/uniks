/**
 * @file sbi.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-01-29 16:18:49
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */

#include "sbi.h"
#include <uniks/defs.h>


int64_t sbi_call(uint64_t sbi_type, ...)
{
	va_list ap;
	va_start(ap, sbi_type);
	register uint64_t a0 asm("a0") = va_arg(ap, uint64_t);
	register uint64_t a1 asm("a1") = va_arg(ap, uint64_t);
	register uint64_t a2 asm("a2") = va_arg(ap, uint64_t);
	register uint64_t a6 asm("a6") = va_arg(ap, uint64_t);
	register uint64_t a7 asm("a7") = sbi_type;
	va_end(ap);
	asm volatile("ecall"
		     : "=r"(a0)
		     : "r"(a0), "r"(a1), "r"(a2), "r"(a6), "r"(a7)
		     : "memory");
	return a0;
}

__always_inline int64_t sbi_set_timer(uint64_t stime_value)
{
	return sbi_call(SBI_SET_TIMER, stime_value);
}

int64_t sbi_console_putchar(char ch)
{
	return sbi_call(SBI_CONSOLE_PUTCHAR, ch);
}

int64_t sbi_console_getchar()
{
	return sbi_call(SBI_CONSOLE_GETCHAR);
}

void sbi_shutdown()
{
	sbi_call(SBI_SHUTDOWN);
}

struct sbiret sbi_hart_start(uint64_t hartid, uint64_t start_addr,
			     uint64_t opaque)
{
	sbi_call(SBI_HSM, hartid, start_addr, opaque, HART_START);
	register uint64_t a0 asm("a0");
	register uint64_t a1 asm("a1");
	struct sbiret sr = {a0, a1};
	return sr;
}