#ifndef __LIBS_KSTDIO_H__
#define __LIBS_KSTDIO_H__


#include <sync/spinlock.h>
#include <uniks/defs.h>


// lock to avoid interleaving concurrent printf's.
struct kprintf_sync_t {
	struct spinlock_t lock;
	int32_t locking;
};

extern struct kprintf_sync_t pr;


void kprintfinit();
void kprintf(const char *fmt, ...);
void kputc(char c);
void kputs(const char *str);
char getchar();


#endif /* !__LIBS_KSTDIO_H__ */
