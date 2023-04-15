# trap.S

	#
	# interrupts and exceptions while in supervisor mode come here.
	# and the current stack is a kernel stack.
	#

#include <uniks/asm.inc>

.global kerneltrapvec
.extern kerneltrap_handler

.section .text
.align 4
kerneltrapvec:
	# note: only save caller save regs
	# make room to save registers
	addi sp, sp, -128
	caller_reg_save sp
	# call the C kerneltrap_handler in trap.c
	call kerneltrap_handler

	# restore context registers
	caller_reg_restore sp
	addi sp, sp, 128

	# return to whatever we were doing in the kernel
	sret