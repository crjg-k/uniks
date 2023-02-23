#ifndef __KERNEL_PROCESS_PROC_H__
#define __KERNEL_PROCESS_PROC_H__

#include "param.h"
#include <defs.h>

#define PROC_NAME_LEN 63
#define MAX_PROCESS   4096
#define MAX_PID	      (MAX_PROCESS * 2)


struct context {
	/*   0 */ uint64_t ra;
	/*   8 */ uint64_t sp;
	/*  16 */ uint64_t gp;
	/*  24 */ uint64_t tp;
	/*  32 */ uint64_t t0;
	/*  40 */ uint64_t t1;
	/*  48 */ uint64_t t2;
	/*  56 */ uint64_t s0;
	/*  64 */ uint64_t s1;
	/*  72 */ uint64_t a0;
	/*  80 */ uint64_t a1;
	/*  88 */ uint64_t a2;
	/*  96 */ uint64_t a3;
	/* 104 */ uint64_t a4;
	/* 112 */ uint64_t a5;
	/* 120 */ uint64_t a6;
	/* 128 */ uint64_t a7;
	/* 136 */ uint64_t s2;
	/* 144 */ uint64_t s3;
	/* 152 */ uint64_t s4;
	/* 160 */ uint64_t s5;
	/* 168 */ uint64_t s6;
	/* 176 */ uint64_t s7;
	/* 184 */ uint64_t s8;
	/* 192 */ uint64_t s9;
	/* 200 */ uint64_t s10;
	/* 208 */ uint64_t s11;
	/* 216 */ uint64_t t3;
	/* 224 */ uint64_t t4;
	/* 232 */ uint64_t t5;
	/* 240 */ uint64_t t6;
	/* 248 */ uint64_t pc;
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
	int32_t pid;			// Process ID
	enum procstate state;		// Process state
	uintptr_t kstack;		// Process kernel stack
	struct proc *parent;		// parent process
	pagetable_t pagetable;		// User page table
	struct context ctxt;		// Switch here to run process
	struct trapframe *tf;		// Trap frame for current interrupt
	char name[PROC_NAME_LEN + 1];	// Process name
};


void proc_init();

#endif /* !__KERNEL_PROCESS_PROC_H__ */