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
#include <stdarg.h>

// SBI number
#define SBI_SET_TIMER	    0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI	    3
#define SBI_SEND_IPI	    4
#define SBI_SHUTDOWN	    8

int64_t sbi_call(uint64_t sbi_type, ...)
{
	va_list ap;
	va_start(ap, sbi_type);
	register uint64_t a0 asm("a0") = va_arg(ap, uint64_t);
	register uint64_t a1 asm("a1") = va_arg(ap, uint64_t);
	register uint64_t a2 asm("a2") = va_arg(ap, uint64_t);
	register uint64_t a7 asm("a7") = sbi_type;
	va_end(ap);
	asm volatile("ecall"
		     : "=r"(a0)
		     : "r"(a0), "r"(a1), "r"(a2), "r"(a7)
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
