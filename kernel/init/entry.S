# entry.S

#include <mmu.h>
#include <memlay.h>

.global kernel_entry

# The ,"ax",@progbits tells the assembler that the section is allocatable ("a"),
# executable ("x") and contains data ("@progbits").
# https://www.nongnu.org/avr-libc/user-manual/mem_sections.html
.section .text, "ax", @progbits
kernel_entry:
	la	sp, bootstacktop	# set sp pointer

	tail	kernel_start


.section .data
	.align PGSHIFT			# align to next page
	.global bootstack
bootstack:
	.space KSTACKSIZE		# stack size = KSTACKSIZE
	.global bootstacktop
bootstacktop: