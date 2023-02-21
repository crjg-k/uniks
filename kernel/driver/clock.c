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
#include <platform/riscv.h>
#include <platform/sbi.h>

volatile uint64_t ticks;

#define TIMESPERSEC 100
#define CPUFREQ	    10000000

// Hardcode timebase
static uint64_t timebase = CPUFREQ / TIMESPERSEC;

__always_inline static uint64_t get_cycles()
{
	uint64_t n;
	asm volatile("rdtime %0" : "=r"(n));
	return n;
}

void clock_set_next_event()
{
	sbi_set_timer(get_cycles() + timebase);
}

void clock_init()
{
	// enable timer interrupt in sie
	set_csr(sie, MIP_STIP);
	ticks = 0;
	clock_set_next_event();
}