
#ifndef __LIBS_KSTDIO_H__
#define __LIBS_KSTDIO_H__

#include <stdarg.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>


void kprintf(const char *fmt, ...);
void kputc(char c);
void kputs(const char *str);
char getchar();

#endif /* !__LIBS_KSTDIO_H__ */
