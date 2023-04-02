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
#include <param.h>
#include <platform/riscv.h>
#include <platform/sbi.h>
#include <process/proc.h>

// the ticks will inc in each 0.01s
uint64_t ticks = 0;
struct spinlock tickslock;

// Hardcode timebase
static uint64_t timebase = CPUFREQ / TIMESPERSEC;

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
	initlock(&tickslock, "tickslock");
	// enable timer interrupt in sie
	set_csr(sie, MIP_STIP);
	ticks = 0;
	clock_set_next_event();
}

/**
 * @brief only the primary hart could cope with the timer interrupt, so we
 * can simply disable the interrupt to avoid dead lock
 */
void clock_interrupt_handler()
{
	// note: may need a lock here, but it's pretty easy to occur a dead lock
	ticks++;
	clock_set_next_event();
	// wakeup(&ticks);
}