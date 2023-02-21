
#include <driver/clock.h>
#include <kstdio.h>
#include <platform/riscv.h>
#include "trap.h"

extern void trap_vector();
void trap_init()
{
	/* Set sscratch to 0, indicating to exception vector that we are
	 * presently executing is from the kernel while >0 means from usermode
	 */
	write_csr(sscratch, 0);
	/* Set the exception vector address */
	write_csr(stvec, &trap_vector);
}

static uint64_t interrupt_handler(uint64_t epc, uint64_t cause)
{
	cause = (cause << 1) >> 1;   // erase the MSB
	switch (cause) {
	case IRQ_U_SOFT:
		kprintf("User software interrupt\n");
		break;
	case IRQ_S_SOFT:
		kprintf("Supervisor software interrupt\n");
		break;
	case IRQ_U_TIMER:
		kprintf("User timer interrupt\n");
		break;
	case IRQ_S_TIMER:
		clock_set_next_event();
#define TICK_NUM 100
		if (++ticks % TICK_NUM == 0)
			kprintf("[%d ticks]\n", ticks);
		break;
	case IRQ_U_EXT:
		kprintf("User external interrupt\n");
		break;
	case IRQ_S_EXT:
		kprintf("Supervisor external interrupt\n");
		break;
	default:
		kprintf("default interrupt\n");
	}
	return epc;
}
static uint64_t exception_handler(uint64_t epc, uint64_t cause)
{
	return epc;
}

uint64_t trap_handler(uint64_t epc, uint64_t cause)
{
	uint64_t return_pc = epc;
	if ((int64_t)cause < 0)
		return_pc = interrupt_handler(epc, cause);
	else
		return_pc = exception_handler(epc, cause);

	return return_pc;
}

__always_inline void interrupt_enable()
{
	set_csr(sstatus, SSTATUS_SIE);
}

__always_inline void interrupt_disable()
{
	clear_csr(sstatus, SSTATUS_SIE);
}