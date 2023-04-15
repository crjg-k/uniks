# trampoline.S

#include <mm/memlay.h>
#include <uniks/asm.inc>


.global trampoline
.global usertrapvec
.extern usertrap_handler

.section .text
.align 4
trampoline:
usertrapvec:
	# note: when trap from user space, the addr of this section start at 0x3ffffff000 while label usertrapvec locate at 0x3ffffff600
	csrw sscratch, t6

	# save all registers to p->tf
	li t6, TRAPFRAME
	tf_reg_save t6

	# save t6 to p->tf
	csrr t5, sscratch
	sd t5, 240(t6)

	ld sp, 256(t6)		# set kernel stack pointer, from p->tf->kernel_sp
	ld tp, 280(t6)		# make tp hold the current hartid, from p->tf->kernel_hartid
	ld t0, 264(t6)		# load the address of usertrap_handler(), from p->tf->kernel_trap
	ld t1, 248(t6)		# fetch the kernel page table address, from p->tf->kernel_satp

	# switch to kernel page table
	sfence.vma zero, zero
	csrw satp, t1
	sfence.vma zero, zero

	# jump to usertrap_handler(), which does not return
	jr t0

# userret(pagetable)
.global userret
# note: when kernel step here to regain user space, the addr of this section start at 0x3ffffff000
# note: so you couldn't cause a break point by the symbol name 'userret'
userret:
	# a0: user page table

	# switch to user page table
	sfence.vma zero, zero
	csrw satp, a0
	sfence.vma zero, zero

	# restore all registers from TRAPFRAME
	li t6, TRAPFRAME
	tf_reg_restore t6

	# return to user mode and jump to user pc
	# usertrapret() has set up sstatus and sepc in front
	sret
