# trap.S

#include <asm.inc>

.global trap_vector

.section .text
# the trap vector base address must always be aligned on a 4-byte boundary
.align 4
trap_vector:
	# save context registers expect t6
	csrrw	t6, sscratch, t6	# swap t6 and sscratch
	reg_save t6
	# save t6
	csrr	t5, sscratch
	sd	t5, 240(t6)

	csrr	a0, sepc
	sd	a0, 248(t6)

	csrrw	t6, sscratch, t6

	# prepare the argu of C function trap_handler
	csrr	a1, scause
	call	trap_handler

	# trap_handler will return the return address via a0
	csrw	sepc, a0

	# restore context registers
	csrr	t6, sscratch
	reg_restore t6

	sret