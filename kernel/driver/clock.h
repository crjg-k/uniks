#ifndef __KERNEL_DRIVER_CLOCK_H__
#define __KERNEL_DRIVER_CLOCK_H__

#include <defs.h>
#include <sync/spinlock.h>


extern volatile uint64_t ticks;
extern struct spinlock tickslock;

void clock_set_next_event();
void clock_init();
void clock_interrupt_handler(int32_t prilevel);

#endif /* !__KERNEL_DRIVER_CLOCK_H__ */
