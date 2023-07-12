#include "trap.h"
#include <device/clock.h>
#include <mm/memlay.h>
#include <mm/vm.h>
#include <platform/plic.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <uniks/kassert.h>
#include <uniks/kstdio.h>
#include <uniks/log.h>

extern void kerneltrapvec(), scheduler(), syscall(), yield();
extern char trampoline[], usertrapvec[], userret[];
extern volatile uint64_t ticks;
void usertrap_handler();

void trap_init()
{
	write_csr(stvec, &kerneltrapvec);
}

static void interrupt_handler(uint64_t cause, int32_t prilevel)
{
	assert(myproc()->magic == UNIKS_MAGIC);
	cause = (cause << 1) >> 1;   // erase the MSB
	switch (cause) {
	case IRQ_U_SOFT:
		kprintf("User software interrupt");
		break;
	case IRQ_S_SOFT:
		kprintf("Supervisor software interrupt");
		break;
	case IRQ_U_TIMER:
		kprintf("User timer interrupt");
		break;
	case IRQ_S_TIMER:
		// hint: there may be a wrong that the yield is not equidistant
		clock_interrupt_handler(prilevel);
		break;
	case IRQ_U_EXT:
		kprintf("User external interrupt");
		break;
	case IRQ_S_EXT:
		external_interrupt_handler();	// external device
		break;
	default:
		panic("default interrupt");
	}
}
static void exception_handler(uint64_t cause, struct proc_t *p,
			      int32_t prilevel)
{
	assert(myproc()->magic == UNIKS_MAGIC);
	switch (cause) {
	case EXC_INST_ILLEGAL:
		tracef("illegal instruction from prilevel: %d, addr: %p",
		       prilevel, p->tf->epc);
		p->tf->epc += 4;
		break;
	case EXC_U_ECALL:   // system call
		tracef("process: %d, system call: %d", p->pid, p->tf->a7);
		/**
		 * @note: this inc must lay before syscall() since that the fork
		 * syscall need the right instruction address
		 */
		p->tf->epc += 4;
		syscall();
		break;
	case EXC_INST_PAGEFAULT:
		tracef("process: %d, inst page fault from prilevel: %d, addr: %p\n",
		       p->pid, prilevel, p->tf->epc);
		do_inst_page_fault(p->tf->epc);
		break;
	case EXC_LD_PAGEFAULT:
		tracef("process: %d, ld page fault from prilevel: %d, addr: %p\n",
		       p->pid, prilevel, read_csr(stval));
		do_ld_page_fault(read_csr(stval));
		break;
	case EXC_SD_PAGEFAULT:
		tracef("process: %d, sd page fault from prilevel: %d, addr: %p\n",
		       p->pid, prilevel, read_csr(stval));
		do_sd_page_fault(read_csr(stval));
		break;
	default:
		kprintf("exception_handler(): unexpected scause %p pid=%d",
			cause, p->pid);
		kprintf("\tepc=%p, from prilevel: %d\n", p->tf->epc, prilevel);
	}
}

// return to user space
void usertrapret()
{
	struct proc_t *p = myproc();

	// note: the 2nd operand should change according to if using OpenSBI
	uint64_t trampoline_uservec =
		TRAMPOLINE + (usertrapvec - KERNEL_BASE_ADDR) % PGSIZE;

	/**
	 * @brief we're about to switch the destination of traps from
	 * kerneltrap() to usertrap_handler(), so turn off interrupts until
	 * we're back in user space, where usertrap_handler() is correct
	 */
	interrupt_off();

	/**
	 * @brief we will return to user space, so the traps are supposed to be
	 * tackled with usertrapvec
	 */
	write_csr(stvec, trampoline_uservec);

	/**
	 * @brief set up trapframe values that usertrapvec will need when the
	 * process next traps into the kernel
	 */
	p->tf->kernel_satp = read_csr(satp);   // kernel page table
	p->tf->kernel_sp = p->kstack;	       // process's kernel stack
	p->tf->kernel_trap = (uint64_t)usertrap_handler;

	/**
	 * @brief set up the registers that trampoline.S's sret will use to
	 * return to user space
	 */
	clear_csr(sstatus, SSTATUS_SPP);   // clear SPP to 0 for user mode
	set_csr(sstatus, SSTATUS_SPIE);	   // enable interrupts in user mode

	// set sepc to the saved user pc
	write_csr(sepc, p->tf->epc);

	// tell trampoline.S the user page table to switch to.
	uint64_t satp = MAKE_SATP(p->pagetable);

	// note: the 2nd operand should change according to if using OpenSBI
	uint64_t trampoline_usertrapret =
		TRAMPOLINE + (uint64_t)(userret - KERNEL_BASE_ADDR) % PGSIZE;
	p->jiffies = ticks;
	// jmp to userret in trampoline.S
	((void (*)(uint64_t))trampoline_usertrapret)(satp);
}

/**
 * @brief handle an interrupt, exception, or system call from user space called
 * from trampoline.S
 */
void usertrap_handler()
{
	// assert that this trap is from user mode
	assert((read_csr(sstatus) & SSTATUS_SPP) == 0);

	/**
	 * @brief now in kernel, so the traps are supposed to be tackled with
	 * kerneltrapvec
	 */
	write_csr(stvec, &kerneltrapvec);

	struct proc_t *p = myproc();
	p->tf->epc = read_csr(sepc);   // save user's pc

	int64_t cause = read_csr(scause);

	/**
	 * @brief an interrupt will change sepc, scause, and sstatus, so enable
	 * only now that we're done with those registers
	 */
	interrupt_on();

	if (cause < 0)
		interrupt_handler(cause, PRILEVEL_U);
	else
		exception_handler(cause, p, PRILEVEL_U);

	usertrapret();
}

/**
 * @brief handle an interrupt, exception, or system call from S mode called from
 * kerneltrap.S
 */
void kerneltrap_handler()
{
	// assert that from S mode
	assert((read_csr(sstatus) & SSTATUS_SPP) != 0);

	int64_t cause = read_csr(scause);
	// assume that in kernel space, no exception will occur
	assert(cause < 0);
	if (cause < 0)
		interrupt_handler(cause, PRILEVEL_S);
	else
		exception_handler(cause, myproc(), PRILEVEL_S);
}
