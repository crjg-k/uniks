#ifndef __KERNEL_DRIVER_CLOCK_H__
#define __KERNEL_DRIVER_CLOCK_H__

#include <defs.h>


extern volatile uint64_t ticks;

void clock_set_next_event();
void clock_init();
/**
 * @brief the precision is 1ms
 */
void delay(int32_t);

#endif /* !__KERNEL_DRIVER_CLOCK_H__ */
