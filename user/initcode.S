#include <syscall.h>

.global _start

.section .text
_start:
	li a7, SYS_fork
	ecall

## if(fork()==0) info[10]='2';
	bne a0, zero, exit
	la a1, info
	li a2, '2'
	sb a2, 9(a1)

# while(1) ;
exit:
	la a1, info
	li a2, 12
	li a7, SYS_write
	ecall
	j exit

# char info[] = "initproc 1\n\0";
.section .data
info:
	.string "initproc 1\n\0"
