#ifndef __KERNEL_PROCESS_PROC_H__
#define __KERNEL_PROCESS_PROC_H__

#include <defs.h>

#define PROC_NAME_LEN 63
#define MAX_PROCESS   4096
#define MAX_PID	      (MAX_PROCESS * 2)


struct context {
	uint64_t ra;
	uint64_t sp;
	uint64_t s0;
	uint64_t s1;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
};

enum procstate {
	UNUSED,
	INITING,
	SLEEPING,
	READY,
	RUNNING,
	ZOMBIE
};

struct proc {
	enum procstate state;		// Process state
	int32_t pid;			// Process ID
	uintptr_t kstack;		// Process kernel stack
	struct proc *parent;		// parent process
	pagetable_t pagetable;		// User page table
	struct context context;		// Switch here to run process
	struct trapframe *tf;		// Trap frame for current interrupt
	char name[PROC_NAME_LEN + 1];	// Process name
};

void proc_init();

#endif /* !__KERNEL_PROCESS_PROC_H__ */