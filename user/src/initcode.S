#include <usyscall.h>

/**
 * The writing format for the read-only data segment just below comes from
 * the online compiler explorer:
 * https://godbolt.org/
 */


.global _start


.section .rodata
# char file[] = "/bin/initrc";
file:
	.string "/bin/initrc"

# char *argv[]={"initrc", "hello", "uniks", "world", NULL};
.LC0:
	.string "hello"
.LC1:
	.string "uniks"
.LC2:
	.string "world"
.p2align 3			// 8-byte alignment of pointer variables under 64 bit
argv:
	.dword file
	.dword .LC0
	.dword .LC1
	.dword .LC2
	.dword 0

# char *envp[]={"PATH=/bin", "HOME=/root", "SHELL=/bin/ksh", NULL};
.LC3:
	.string "PATH=/bin"
.LC4:
	.string "HOME=/root"
.LC5:
	.string "SHELL=/bin/ksh"
.p2align 3			// 8-byte alignment of pointer variables under 64 bit
envp:
	.dword .LC3
	.dword .LC4
	.dword .LC5
	.dword 0

.section .text
_start:
# execve(file, argv, envp);
	li a7, SYS_execve
	la a0, file
	la a1, argv
	la a2, envp
	ecall

# exit(-1);
	li a7, SYS_exit
	li a0, -1
	ecall
