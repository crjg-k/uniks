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
#include <process/proc.h>
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/param.h>

// the ticks will inc in each 0.001s namely 1ms
volatile uint64_t ticks = 0;
struct spinlock_t tickslock;

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
	initlock(&tickslock, "tickslock");
	set_csr(sie, MIP_STIP);	  // enable timer interrupt in sie
	clock_set_next_event();
}

/**
 * @brief only the primary hart could cope with the timer interrupt, so we
 * can simply disable the interrupt to avoid dead lock
 */
__always_inline void clock_interrupt_handler(int32_t prilevel)
{
	acquire(&tickslock);   // this have disabled interrupt interior
	ticks++;
	clock_set_next_event();
	time_wakeup();
	release(&tickslock);

	if (prilevel == PRILEVEL_U) {
		struct proc_t *p = myproc();
		acquire(&pcblock[p->pid]);
		p->jiffies = ticks;
		p->ticks--;
		if (!p->ticks) {
			p->ticks = p->priority;
			release(&pcblock[p->pid]);
			yield();
		} else
			release(&pcblock[p->pid]);
	}
	assert(myproc()->magic == UNIKS_MAGIC);
}