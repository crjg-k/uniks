#ifndef __KERNEL_PROCESS_PROC_H__
#define __KERNEL_PROCESS_PROC_H__

#include <defs.h>
#include <list.h>
#include <param.h>
#include <priority_queue.h>
#include <queue.h>
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

enum proc_state {
	TASK_UNUSED,
	TASK_INITING,
	TASK_BLOCK,
	TASK_READY,
	TASK_RUNNING,
	TASK_ZOMBIE
};

struct proc {
	// this->lock must be held when using these below:
	int32_t pid;		 // process ID
	enum proc_state state;	 // process state
	int8_t killed;		 // if non-zero, have been killed
	int32_t exitstate;	 // exit status to be returned to parent's wait
	uint32_t priority;	 // process priority
	uint32_t ticks;		 // remainder time slices
	uint32_t jiffies;	 // global time slice when last execution
	struct list_node block_list;   // block list of this process
	struct list_node wait_list;    // who wait for this process to exit

	uintptr_t kstack;    // process kernel stack, always point to kstack top
	uint64_t sz;	     // size of process memory (bytes)
	int32_t parentpid;   // parent process
	pagetable_t pagetable;	 // user page table
	struct context ctxt;	 // switch here to run process
	/**
	 * @brief the trapframe for current interrupt, and furthermore, it point
	 * to the beginning of kstack at the same time. The layout of kstack:
	 *
	 * kernelstacktop ->    +---------------+<=====+ (high address)
	 *                      |               |      |
	 *                      |               |      |
	 *                      |               |      |
	 *                 +----+---------------+      |
	 *                 |    |     MAGIC     |      |
	 *                 |    +---------------+      |
	 *                 |    |      ...      |      |
	 *                 |    +---------------+      |
	 *                 |    |      TF       |---+  |           +---+---+---+
	 * STRUCT PROC <---|    +---------------+   |  |           |🔒 |...| 🔒|
	 *                 |    |      ...      |   |  |  pcblock: +---+---+---+
	 *                 |    +---------------+   |  |             |       |
	 *                 |    |    KSTACK     |===|==+             ⬇       ⬇
	 *                 |    +---------------+   |              +---+---+---+
	 *                 |    |      ...      |   |              |   |...|   |
	 *                 +----+---------------+<--|----pcbtable: +---+---+---+
	 *                      |               |   |
	 *                      |   TRAPFRAME   |   |
	 *                      |               |   |
	 * kernelstackbottom -> +---------------+<--+ (low address)
	 */
	struct trapframe *tf;
	char name[PROC_NAME_LEN + 1];	// process name

	uint32_t magic;	  // magic number as canary to determine stackoverflow
};

struct cpu {
	struct proc *proc;     // The process running on this cpu, or null
	struct context ctxt;   // swtch() here to enter scheduler()
	uint32_t repeat;       // reacquire lock times per-cpu
	uint64_t preintstat;   // pre-interrupt enabled status before push_off()
};

struct pids_queue_t {
	struct spinlock pid_lock;
	struct queue_meta qm;
	int32_t pids_queue_array[NPROC];
};

struct sleep_queue_t {
	struct spinlock sleep_lock;
	struct priority_queue_meta pqm;
	struct pair sleepqueue[NPROC];
};

extern struct proc *pcbtable[];
extern struct spinlock pcblock[];
extern struct cpu cpus[];

void scheduler();
void sched();
void proc_init();
void user_init(uint32_t priority);
void yield();
void time_wakeup();
int8_t killed(struct proc *);
struct cpu *mycpu();
struct proc *myproc();
void proc_block(struct proc *p, struct list_node *list, enum proc_state state);
void proc_unblock(struct proc *p);


// process relative syscall
int64_t do_fork();
int64_t do_exec();
void do_exit(int32_t status);

#endif /* !__KERNEL_PROCESS_PROC_H__ */