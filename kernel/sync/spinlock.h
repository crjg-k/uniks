#ifndef __KERNEL_SYNC_SPINLOCK_H__
#define __KERNEL_SYNC_SPINLOCK_H__


#include <uniks/defs.h>


struct spinlock_t {
	volatile uint32_t locked;
	int32_t cpu;   // which cpu holding the lock
	// for debugging:
	char *name;   // name of the lock
};

void initlock(struct spinlock_t *lk, char *name);
int64_t holding(struct spinlock_t *lk);

void push_off();
void pop_off();

void do_acquire(struct spinlock_t *lk);
void do_release(struct spinlock_t *lk);


#define acquire(lk) ({ do_acquire(lk); })
#define release(lk) ({ do_release(lk); })


#endif /* !__KERNEL_SYNC_SPINLOCK_H__ */