#include <usyscall.h>

/**
 * The writing format for the read-only data segment just below comes from
 * the online compiler explorer:
 * https://godbolt.org/
 */


.global _start


.section .rodata
# char file[] = "/bin/init";
file:
	.string "/bin/init"
# char *argv[]={"hello", "uniks", "world", NULL};
.LC0:
	.string "hello"
.LC1:
	.string "uniks"
.LC2:
	.string "world"
argv:
	.dword .LC0
	.dword .LC1
	.dword .LC2
	.dword 0
# char *envp[]={"HOME=/root", "PATH=/bin", NULL};
.LC3:
	.string "HOME=/root"
.LC4:
	.string "PATH=/bin"
envp:
	.dword .LC3
	.dword .LC4
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
