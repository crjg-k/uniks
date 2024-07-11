#include "trap.h"
#include <device/clock.h>
#include <mm/memlay.h>
#include <mm/vm.h>
#include <platform/plic.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/kstdio.h>
#include <uniks/log.h>


extern char kerneltrapvec[], trampoline[], usertrapvec[], userret[];
uint64_t trampoline_uservec, trampoline_usertrapret;
char *fault_msg[] = {
	"Instruction address misaligned",
	"Instruction access fault",
	"Illegal instruction",
	"Breakpoint",
	"Load address misaligned",
	"Load access fault",
	"Store/AMO address misaligned",
	"Store/AMO access fault",
	"",
	"Environment call from Supervisor-mode",
};

void trap_inithart()
{
	write_csr(stvec, &kerneltrapvec);
}

void trap_init()
{
	trampoline_uservec = TRAMPOLINE + OFFSETPAGE(usertrapvec - trampoline);
	trampoline_usertrapret = TRAMPOLINE + OFFSETPAGE(userret - trampoline);
}

static void interrupt_handler(uint64_t cause)
{
	assert(myproc()->magic == UNIKS_MAGIC);
	clear_var_bit(cause, INT64_MIN);   // erase the MSB
	switch (cause) {
	case IRQ_S_SOFT:
		kprintf("Supervisor software interrupt");
		break;
	case IRQ_S_TIMER:
		clock_set_next_event();
		clock_interrupt_handler();
		break;
	case IRQ_S_EXT:
		external_interrupt_handler();	// external device
		break;
	default:
		BUG();
	}
}

static void exception_handler(uint64_t cause, uint64_t stval, struct proc_t *p)
{
	assert(p->magic == UNIKS_MAGIC);
	switch (cause) {
	case EXC_U_ECALL:   // system call
		tracef("process: %d, system call: %d", p->pid, p->tf->a7);
		if (killed(p))
			do_exit(-1);
		/**
		 * @note: this inc must lay before syscall() since that the fork
		 * syscall need the right instruction address
		 */
		p->tf->epc += 4;
		syscall();
		break;
	case EXC_INST_PAGEFAULT:
		tracef("process: %d, instruction page fault, addr: %p\n",
		       p->pid, p->tf->epc);
		do_inst_page_fault(p->tf->epc);
		break;
	case EXC_LD_PAGEFAULT:
		tracef("process: %d, ld page fault, addr: %p=>%p\n", p->pid,
		       p->tf->epc, stval);
		do_ld_page_fault(stval);
		break;
	case EXC_SD_PAGEFAULT:
		tracef("process: %d, sd page fault, addr: %p=>%p\n", p->pid,
		       p->tf->epc, stval);
		do_sd_page_fault(stval);
		break;
	default:
		kprintf("\n\x1b[%dmpid[%d] unexpected scause[%d]:%p=>%s\x1b[0m\n",
			RED, p->pid, cause, p->tf->epc, fault_msg[cause]);
		setkilled(p);
	}
}

// return to user space
void usertrapret()
{
	/**
	 * @brief we're about to switch the destination of traps from
	 * kerneltrap() to usertrap_handler(), so turn off interrupts until
	 * we're back in user space, where usertrap_handler() is correct
	 */
	interrupt_off();

	struct proc_t *p = myproc();

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
	p->tf->kernel_sp = p->mm->kstack;      // process's kernel stack
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
	uint64_t satp = MAKE_SATP(p->mm->pagetable, p->pid);

	// jmp to userret in trampoline.S
	((void (*)(uint64_t))trampoline_usertrapret)(satp);
}

/**
 * @brief handle an interrupt, exception, or system call from user space called
 * from trampoline.S
 */
void usertrap_handler()
{
	// assert that this trap is from U mode
	assert(get_var_bit(read_csr(sstatus), SSTATUS_SPP) == 0);

	/**
	 * @brief now in kernel, so the traps are supposed to be tackled with
	 * kerneltrapvec
	 */
	write_csr(stvec, &kerneltrapvec);

	struct proc_t *p = myproc();
	p->tf->epc = read_csr(sepc);   // save user's pc

	int64_t cause = read_csr(scause), stval = read_csr(stval);

	/**
	 * @brief an interrupt will change sepc, scause, and sstatus, so enable
	 * only now that we're done with those registers
	 */
	interrupt_on();

	if (cause < 0)
		interrupt_handler(cause);
	else
		exception_handler(cause, stval, p);

	if (killed(p))
		do_exit(-1);

	{
		acquire(&pcblock[p->pid]);
		p->jiffies = atomic_load(&ticks);
		p->ticks--;
		if (!p->ticks) {
			assert(p->state == TASK_RUNNING);
			p->ticks = p->priority;
			yield();
		} else
			release(&pcblock[p->pid]);
	}

	usertrapret();
}

/**
 * @brief handle an interrupt, exception, or system call from S mode called from
 * kerneltrap.S
 */
void kerneltrap_handler()
{
	// assert that from S mode
	assert(get_var_bit(read_csr(sstatus), SSTATUS_SPP) != 0);

	int64_t cause = read_csr(scause);
	// assume that in kernel space, no exception will occur
	if (cause < 0)
		interrupt_handler(cause);
	else {
		panic("%s(): unexpected scause[%x] hartid=%x<=>sepc=%p, stval=%p\n",
		      __func__, cause, cpuid(), read_csr(sepc),
		      read_csr(stval));
	}
}
