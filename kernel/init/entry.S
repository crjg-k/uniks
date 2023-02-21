# entry.S

#include <mm/mmu.h>
#include <mm/memlay.h>

.global kernel_entry


.section .text
kernel_entry:
	# now, in S mode!
	la	sp, bootstacktop	# set sp pointer
	tail	kernel_start


.section .data
	.align PGSHIFT			# align to next page
	.global bootstack
bootstack:
	.space KSTACKSIZE		# stack size = KSTACKSIZE
	.global bootstacktop
bootstacktop:
