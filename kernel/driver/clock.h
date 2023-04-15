#ifndef __KERNEL_DRIVER_CLOCK_H__
#define __KERNEL_DRIVER_CLOCK_H__

#include <sync/spinlock.h>
#include <uniks/defs.h>


extern volatile uint64_t ticks;
extern struct spinlock_t tickslock;

void clock_set_next_event();
void clock_init();
void clock_interrupt_handler(int32_t prilevel);

#endif /* !__KERNEL_DRIVER_CLOCK_H__ */
