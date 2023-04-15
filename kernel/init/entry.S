# entry.S

#include <mm/mmu.h>
#include <mm/memlay.h>

.global kernel_entry


.section .text
kernel_entry:
	# now, in S mode!
	mv tp, a0			# this argu is passed by OpenSBI

	la sp, bootstacktop0		# set sp pointer
	tail kernel_start


.section .data
	.align PGSHIFT			# align to next page
	.global bootstack0
bootstackbottom0:
	.space KSTACKSIZE		# stack size = KSTACKSIZE
	.global bootstacktop0
bootstacktop0:

.section .data
	.align PGSHIFT			# align to next page
	.global bootstack1
bootstackbottom1:
	.space KSTACKSIZE		# stack size = KSTACKSIZE
	.global bootstacktop1
bootstacktop1:
