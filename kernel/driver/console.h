#ifndef __KERNEL_DRIVER_CONSOLE_H__
#define __KERNEL_DRIVER_CONSOLE_H__

#include <uniks/defs.h>

void consoleinit();
int64_t console_putchar(char c);
int64_t console_getchar();

#endif /* !__KERNEL_DRIVER_CONSOLE_H__ */
