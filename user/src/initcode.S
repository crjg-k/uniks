#include <usyscall.h>

.global _start

.section .text
_start:
	li a7, SYS_fork
	ecall
## if(fork()==0) { info[10]='2';
	bne a0, zero, exit
	la a1, info
	li a2, '2'
	sb a2, 9(a1)

	li a7, SYS_fork
	ecall
## if(fork()==0) { info[10]='3'; }
	bne a0, zero, exit
	la a1, info
	li a2, '3'
	sb a2, 9(a1)
# }

# while(1) { write(info, 12); };
exit:
	la a1, info
	li a2, 11
	li a7, SYS_write
	ecall
	j exit

# char info[] = "initproc 1\n";
.section .data
info:
	.string "initproc 1\n\0"
