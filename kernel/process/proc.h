#ifndef __KERNEL_PROCESS_PROC_H__
#define __KERNEL_PROCESS_PROC_H__


#include <fs/ext2fs.h>
#include <mm/vm.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>
#include <uniks/priority_queue.h>
#include <uniks/queue.h>


/**
 * @brief this is only tiny context for kernel context switch occurred by
 * function calling, so we only need to save the callee saved registers
 */
struct context_t {
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

struct trapframe_t {
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
	/* 248 */ uint64_t kernel_satp;	  // kernel page table
	/* 256 */ uint64_t
		kernel_sp;   // top of process's kernel stack (sp point to)
	/* 264 */ uint64_t kernel_trap;	  // usertrap()
	/* 272 */ uint64_t epc;		  // saved user program counter
};

enum proc_state {
	TASK_UNUSED,
	TASK_INITING,
	TASK_BLOCK,
	TASK_READY,
	TASK_RUNNING,
	TASK_ZOMBIE
};

struct proc_t {
	// this->lock must be held when using these below:
	pid_t pid;   // process ID
	// wait_lock must be held when using this:
	pid_t parentpid;	 // parent process
	struct cpu_t *host;	 // which hart is running this process?
	enum proc_state state;	 // process state
	int8_t killed;		 // if non-zero, have been killed
	int8_t w4child;		 // if non-zero, wait for a child to exit
	int32_t exitstate;	 // exit status to be returned to parent's wait
	uint32_t priority;	 // process priority
	uint32_t ticks;		 // remainder time slices
	uint32_t jiffies;	 // global time slice when last execution
	struct list_node_t block_list;	 // block list of this process
	struct list_node_t wait_list;	 // who wait for this process to exit

	// wait_lock must be held when using this:
	struct list_node_t child_list;
	struct list_node_t parentp;

	char *cwd;
	struct m_inode_t *icwd;	  // Current directory
	int16_t fdtable[NFD];	  // fd table pointing to the index of fcbtable

	/* this mm_struct describe the virtual addr space mapping state */
	struct mm_struct *mm;	// porocto the mm_struct of this process

	struct context_t ctxt;	 // switch here to run process
	/**
	 * @brief the trapframe for current interrupt, and furthermore, it point
	 * to the beginning of kstack at the same time. The layout of kstack:
	 *
	 * kernelstacktop    -> +---------------+<=====+ (high address)
	 *                      |               |      |
	 *    kernel_sp   ->    |               |      |
	 *                      |               |      |
	 *                   +--+---------------+      |
	 *                   |  |     MAGIC     |      |
	 *                   |  +---------------+      |
	 *                   |  |      ...      |      |
	 *                   |  +---------------+      |
	 *                   |  |      TF       |---+  |           +---+---+---+
	 * STRUCT PROC_T <---|  +---------------+   |  |           |🔒 |...| 🔒|
	 *                   |  |      ...      |   |  |  pcblock: +---+---+---+
	 *                   |  +---------------+   |  |             |       |
	 *                   |  |    KSTACK     |===|==+             ⬇       ⬇
	 *                   |  +---------------+   |              +---+---+---+
	 *                   |  |      ...      |   |              |   |...|   |
	 *                   +--+---------------+<--|----pcbtable: +---+---+---+
	 *                      |               |   |
	 *                      |   TRAPFRAME   |   |
	 *                      |               |   |
	 * kernelstackbottom->  +---------------+<--+ (low address)
	 */
	struct trapframe_t *tf;
	char *name;   // elf file name corresponding to this process

	uint32_t magic;	  // magic number as canary to determine stackoverflow
};

struct cpu_t {
	uint32_t hartid;
	struct proc_t *proc;   // which process is running on this cpu, or null
	struct context_t ctxt;	 // swtch() here to enter scheduler()
	uint32_t repeat;	 // reacquire lock times per-cpu
	uint64_t preintstat;   // pre-interrupt enabled status before push_off()
};

struct pids_queue_t {
	struct spinlock_t pid_lock;
	struct queue_meta_t qm;
	int32_t pids_queue_array[NPROC];
};

struct sleep_queue_t {
	struct spinlock_t sleep_lock;
	struct priority_queue_meta_t pqm;
	struct pair_t sleep_queue_array[NPROC];
};

extern struct proc_t *pcbtable[];
#define FIRST_PROC pcbtable[0]
#define INIT_PROC  pcbtable[1]
#define LAST_PROC  pcbtable[NPROC - 1]
extern struct spinlock_t pcblock[];
extern struct spinlock_t wait_lock;
extern struct cpu_t cpus[];
extern struct sleep_queue_t sleep_queue;
extern struct pids_queue_t pids_queue;

void freepid(pid_t pid);
void scheduler(struct cpu_t *c);
void sched();
void switch_to(struct context_t *old, struct context_t *new);
void proc_init();
void user_init(uint32_t priority);
void yield();
void time_wakeup();
void setkilled(struct proc_t *p);
int32_t killed(struct proc_t *);
struct cpu_t *mycpu();
struct proc_t *myproc();
void proc_block(struct list_node_t *wait_list, struct spinlock_t *lk);
int32_t proc_unblock_all(struct list_node_t *wait_list);
int32_t user_basic_pagetable(struct proc_t *p);


// process relative syscall
int64_t do_fork();
void do_exit(int32_t status);


#endif /* !__KERNEL_PROCESS_PROC_H__ */