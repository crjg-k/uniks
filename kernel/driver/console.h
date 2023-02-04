
#ifndef __KERNEL_DRIVER_CONSOLE_H__
#define __KERNEL_DRIVER_CONSOLE_H__
#include <defs.h>

int64_t console_putchar(char c);
int64_t console_getchar(void);

#endif /* !__KERNEL_DRIVER_CONSOLE_H__ */
