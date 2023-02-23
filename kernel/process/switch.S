# switch.S

#include <asm.inc>

.global switch_to

.section .text
switch_to:
	# switch sscratch to point to the context of the next task
	csrw	sscratch, a0
	# set sepc to the pc of the next task
	ld	a1, 248(a0)
	csrw	sepc, a1

	# Restore all GPRs
	# Use t6 to point to the context of the new task
	mv	t6, a0
	reg_restore t6

	sret