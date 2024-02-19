# entry.S

#include <mm/mmu.h>
#include <mm/memlay.h>

.global kernel_entry


.section .text
kernel_entry:
	# now, in S mode!
	# a0(contains hart id) is passed by OpenSBI

	# set up a stack for C language env.
	# hart_stacks is declared in init.c, with a 4096-byte stack per CPU.
	# sp = hart_stacks + (hartid * 4096*4)
	la sp, hart_stacks
	addi t0, a0, 1
	slli t0, t0, 14				# multiply by 4096*4(kstack size)
	add sp, sp, t0

	# jump to kernel_start(uint32_t hartid) in init.c
	j kernel_start

