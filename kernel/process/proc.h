#ifndef __KERNEL_PROCESS_PROC_H__
#define __KERNEL_PROCESS_PROC_H__

#include <defs.h>
#include <param.h>
#include <sync/spinlock.h>


// this is only tiny context for kernel context switch occurred by function
// calling, so we only need to save the callee saved registers
struct context {
	/*   0 */ uint64_t ra;
	/*   8 */ uint64_t sp;
	/*  16 */ uint64_t s0;
	/*  24 */ uint64_t s1;
	/*  32 */ uint64_t s2;
	/*  40 */ uint64_t s3;
	/*  48 */ uint64_t s4;
	/*  56 */ uint64_t s5;
	/*  64 */ uint64_t s6;
	/*  72 */ uint64_t s7;
	/*  80 */ uint64_t s8;
	/*  88 */ uint64_t s9;
	/*  96 */ uint64_t s10;
	/* 104 */ uint64_t s11;
};

struct trapframe {
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
	/* 248 */ uint64_t kernel_satp;	    // kernel page table
	/* 256 */ uint64_t kernel_sp;	    // top of process's kernel stack
	/* 264 */ uint64_t kernel_trap;	    // usertrap()
	/* 272 */ uint64_t epc;		    // saved user program counter
	/* 280 */ uint64_t kernel_hartid;   // saved kernel tp
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
	struct spinlock lock;

	// this->lock must be held when using these below:
	int32_t pid;		// process ID
	enum procstate state;	// process state
	void *sleeplist;	// if non-zero, sleeping on sleeplist
	int8_t killed;		// if non-zero, have been killed
	int32_t exitstate;	// exit status to be returned to parent's wait

	uintptr_t kstack;		// process kernel stack
	uint64_t sz;			// size of process memory (bytes)
	struct proc *parent;		// parent process
	pagetable_t pagetable;		// user page table
	struct context ctxt;		// switch here to run process
	struct trapframe *tf;		// trap frame for current interrupt
	char name[PROC_NAME_LEN + 1];	// process name
};

struct cpu {
	struct proc *proc;     // The process running on this cpu, or null
	struct context ctxt;   // swtch() here to enter scheduler()
	int64_t preintstat;    // pre-interrupt enabled status before push_off()
};

extern struct proc pcbtable[];
extern struct cpu cpus[];

void scheduler();
void proc_init();
void user_init();
void sleep(void *sleeplist, struct spinlock *lk);
void wakeup(void *sleeplist);
int8_t killed(struct proc *);
struct cpu *mycpu();
struct proc *myproc();
void proc_mapstacks(pagetable_t);

// process relative syscall
int64_t do_fork();
int64_t do_exec();

#endif /* !__KERNEL_PROCESS_PROC_H__ */