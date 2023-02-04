
#ifndef __KERNEL_PLATFORM_SBI_H__
#define __KERNEL_PLATFORM_SBI_H__

#include <defs.h>

int64_t sbi_set_timer(uint64_t stime_value);
int64_t sbi_console_putchar(char ch);
int64_t sbi_console_getchar(void);
void sbi_shutdown(void);

#endif /* !__KERNEL_PLATFORM_SBI_H__ */