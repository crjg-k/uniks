/**
 * @file clock.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-02-16 11:46:00
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */

#include "clock.h"
#include <uniks/defs.h>
#include <platform/sbi.h>
#include <process/proc.h>
#include <uniks/kassert.h>
#include <uniks/param.h>

// the ticks will inc in each 0.001s namely 1ms
volatile atomic_uint_least64_t ticks = ATOMIC_VAR_INIT(0);


// hardcode jiffy = 1ms and timebase
uint64_t jiffy = 1000 / TIMESPERSEC, timebase = CPUFREQ / TIMESPERSEC;

__always_inline static uint64_t get_cycles()
{
	uint64_t n;
	asm volatile("rdtime %0" : "=r"(n));
	return n;
}

__always_inline void clock_set_next_event()
{
	sbi_set_timer(get_cycles() + timebase);
}

void clock_init()
{
	clock_set_next_event();
}

/**
 * @brief only the primary hart could cope with the timer interrupt, so we
 * can simply disable the interrupt to avoid dead lock
 */
__always_inline void clock_interrupt_handler()
{
	if (cpuid() == boothartid) {
		atomic_fetch_add(&ticks, 1);
	}
	time_wakeup();

	assert(myproc()->magic == UNIKS_MAGIC);
}