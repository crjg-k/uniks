#ifndef __KERNEL_DEVICE_CLOCK_H__
#define __KERNEL_DEVICE_CLOCK_H__


#include <sync/spinlock.h>
#include <uniks/defs.h>


extern volatile atomic_uint_least64_t ticks;
extern uint64_t jiffy, timebase;


void clock_set_next_event();
void clock_init();
void clock_interrupt_handler();


#endif /* !__KERNEL_DEVICE_CLOCK_H__ */
