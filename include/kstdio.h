
#ifndef __LIBS_KSTDIO_H__
#define __LIBS_KSTDIO_H__

#include <defs.h>
#include <stdarg.h>
#include <sync/spinlock.h>

struct printflock {
	struct spinlock lock;
	int32_t locking;   // prevent panic cannot acquire the lock
};


void printfinit();
void kprintf(const char *fmt, ...);
void kputc(char c);
void kputs(const char *str);
char getchar();

#endif /* !__LIBS_KSTDIO_H__ */
