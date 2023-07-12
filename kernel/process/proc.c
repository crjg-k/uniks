#include "proc.h"
#include <mm/memlay.h>
#include <mm/phys.h>
#include <platform/riscv.h>
#include <sync/spinlock.h>
#include <sys/ksyscall.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>
#include <uniks/list.h>


struct proc_t *pcbtable[NPROC] = {NULL};
struct proc_t idlepcb = {
	0,
	TASK_READY,
	0,
	0,
	0,
	0,
	0,
	{},
	{},
	{0},
	(uintptr_t)&bootstacktop0,
	0,
	0,
	0,
	{},
	NULL,
	"idleproc",
	UNIKS_MAGIC,
};   // learn from Linux-0.11 which also did such this - "hard code initing"
struct spinlock_t pcblock[NPROC];
struct cpu_t cpus[NCPU];

struct pids_queue_t pids_queue;
struct sleep_queue_t sleep_queue;   // call sys_sleep waiting process

struct proc_t *initproc = NULL;	    // init proc

extern char trampoline[];
extern volatile uint64_t ticks;
extern void usertrapret(), verify_area(void *addr, int64_t size);
extern int32_t mappages(pagetable_t pagetable, uintptr_t va, size_t size,
			uintptr_t pa, int32_t perm, int8_t recordref);
extern void switch_to(struct context_t *, struct context_t *);
extern void uvmfirst(pagetable_t, uint32_t *, uint32_t);
extern pagetable_t uvmcreate();
extern void uvmunmap(pagetable_t, uint64_t, uint64_t, int32_t);
extern void uvmfree(pagetable_t, uint64_t);
extern int64_t uvmcopy(pagetable_t old, pagetable_t new, uint64_t sz);
extern int32_t copyout(pagetable_t pagetable, void *dstva, void *src,
		       uint64_t len);

/**
 * @brief Set the proc->name as argument name with the constraint of max length,
 * and it supposed that the caller will pass a legal string with '\0' terminator
 * @param proc
 * @param name
 */
static void set_proc_name(struct proc_t *proc, const char *name)
{
	strncpy(proc->name, name, PROC_NAME_LEN);
}

struct cpu_t *mycpu()
{
	int32_t id = r_mhartid();
	struct cpu_t *c = &cpus[id];
	return c;
}
__always_inline struct proc_t *myproc()
{
	push_off();
	struct proc_t *p = mycpu()->proc;
	pop_off();
	return p;
}

int32_t allocpid()
{
	pid_t pid;
	acquire(&pids_queue.pid_lock);
	pid = *(pid_t *)queue_front_int32type(&pids_queue.qm);
	queue_pop(&pids_queue.qm);
	release(&pids_queue.pid_lock);
	return pid;
}

/**
 * @brief create a basic user page table for a given process only initial with
 * trampoline and trapframe pages
 *
 * @param p
 * @return pagetable_t
 */
pagetable_t proc_makebasicpagetable(struct proc_t *p)
{
	pagetable_t pagetable = uvmcreate();
	if (pagetable == 0)
		return 0;

	/**
	 * @brief map the trampoline.S code at the highest user virtual address.
	 * only S mode can use it, on the way to/from user space, so not PTE_U
	 */
	if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64_t)trampoline,
		     PTE_R | PTE_X, 1) < 0) {
		uvmfree(pagetable, 0);
		return 0;
	}

	// map the trapframe page just below the trampoline page
	if (mappages(pagetable, TRAPFRAME, PGSIZE, (uint64_t)(p->tf),
		     PTE_R | PTE_W, 1) < 0) {
		uvmunmap(pagetable, TRAMPOLINE, 1, 0);
		uvmfree(pagetable, 0);
		return 0;
	}
	return pagetable;
}

// free a process's page table, and free the physical memory it refers to
void proc_freepagetable(pagetable_t pagetable, uint64_t sz)
{
	uvmunmap(pagetable, TRAMPOLINE, 1, 0);
	uvmunmap(pagetable, TRAPFRAME, 1, 0);
	uvmfree(pagetable, sz);
}

/**
 * @brief free kstack of a proc which contains the proc structure represents it
 * and also include user pages (unmap then free physical memory), then release
 * the corresponding pointer in pcbtable.
 * p->lock must be held before calling this function
 * @param p
 */
static void freeproc(pid_t pid)
{
	assert(holding(&pcblock[pid]));
	if (pcbtable[pid]->pagetable)
		proc_freepagetable(pcbtable[pid]->pagetable, pcbtable[pid]->sz);
	pages_free(pcbtable[pid]->tf);
	pcbtable[pid] = NULL;
}

/**
 * @brief allocate a new process and fill the tiny context and return with
 * holding the lock of the new process if allocate a process successfully
 *
 * @return struct proc_t *: new allocate process's pcb entry in pcb table
 */
struct proc_t *allocproc()
{
	pid_t newpid = allocpid();
	if (newpid != -1)
		goto found;
	return NULL;
found:
	acquire(&pcblock[newpid]);
	assert(pcbtable[newpid] == NULL);

	struct trapframe_t *tf;
	uintptr_t kstack;

	/**
	 * @brief allocate a kstack page as well as a trapframe page locate at
	 * the begining of kstack
	 */
	if ((tf = pages_alloc(1)) == NULL) {
		// if there are no enough memory, failed
		release(&pcblock[newpid]);
		return NULL;
	}
	kstack = (uintptr_t)tf;
	pcbtable[newpid] =
		(struct proc_t *)(kstack + sizeof(struct trapframe_t));
	pcbtable[newpid]->state = TASK_INITING;
	pcbtable[newpid]->pid = newpid;
	pcbtable[newpid]->kstack = kstack + PGSIZE;

	INIT_LIST_HEAD(&pcbtable[newpid]->block_list);
	INIT_LIST_HEAD(&pcbtable[newpid]->wait_list);

	pcbtable[newpid]->tf = tf;

	// an empty user page table
	if ((pcbtable[newpid]->pagetable =
		     proc_makebasicpagetable(pcbtable[newpid])) == 0) {
		// if there are no enough memory, failed
		freeproc(pcbtable[newpid]->pid);
		release(&pcblock[newpid]);
		return NULL;
	}

	/**
	 * @brief set new ctxt to start executing at forkret, where returns to
	 * user space. this forges a trap scene for new forked process to
	 * release lock right set sp point to the correct kstack address
	 */
	memset(&pcbtable[newpid]->ctxt, 0, sizeof(pcbtable[newpid]->ctxt));
	void forkret();
	pcbtable[newpid]->ctxt.ra = (uint64_t)forkret;
	pcbtable[newpid]->ctxt.sp = pcbtable[newpid]->kstack;
	pcbtable[newpid]->magic = UNIKS_MAGIC;

	return pcbtable[newpid];
}

// a fork child's 1st scheduling by scheduler() will swtch to forkret
void forkret()
{
	usertrapret();
}

__always_inline void initidleproc()
{
	mycpu()->proc = pcbtable[0] = &idlepcb;
}

// initialize the pcb table lock
void proc_init()
{
	initlock(&pids_queue.pid_lock, "nextpid");
	queue_init(&pids_queue.qm, NPROC, pids_queue.pids_queue_array);
	for (int32_t i = 1; i < NPROC; i++)
		queue_push_int32type(&pids_queue.qm, i);
	for (struct spinlock_t *plk = pcblock; plk < &pcblock[NPROC]; plk++) {
		initlock(plk, "proclock");
	}
	initidleproc();
	initlock(&sleep_queue.sleep_lock, "sleeplock");
	priority_queue_init(&sleep_queue.pqm, NPROC,
			    &sleep_queue.sleep_queue_array);
}

// each hart will hold its local scheduler context
void scheduler()
{
	struct cpu_t *c = mycpu();
	c->proc = pcbtable[0];
	interrupt_on();
	while (1) {
		int32_t i = 1;
		for (struct proc_t *p; i < NPROC; i++) {
			p = pcbtable[i];
			if (p == NULL)
				continue;
			// tracef("idle process running");
			acquire(&pcblock[p->pid]);
			if (p->state == TASK_READY) {
				// tracef("switch to: %d\n", p->pid);
				/**
				 * @brief switch to chosen process. it is the
				 * process's job to release its lock and then
				 * reacquire it before jumping back to us (in
				 * yield())
				 */
				c->proc = p;
				p->state = TASK_RUNNING;
				release(&pcblock[p->pid]);
				interrupt_off();
				switch_to(&c->ctxt, &p->ctxt);
				/**
				 * @brief process is done running for now since
				 * timer interrupt
				 */
				c->proc = pcbtable[0];
				interrupt_on();
			} else
				release(&pcblock[p->pid]);
		}
	}
}

// switch to kernel thread scheduler
void sched()
{
	struct proc_t *p = myproc();

	interrupt_off();
	switch_to(&p->ctxt, &mycpu()->ctxt);
	interrupt_on();
}

/**
 * @brief give up the CPU who called this
 */
void yield()
{
	struct proc_t *p = myproc();
	acquire(&pcblock[p->pid]);
	p->state = TASK_READY;
	release(&pcblock[p->pid]);
	sched();
}

/**
 * @brief Make process p blocked and mount it to wait_list list. Furthermore,
 * the lock of the wait_list is supposed to be held before call this function.
 * @param p
 * @param block_list
 * @param state
 */
__always_inline void proc_block(struct proc_t *p, struct list_node_t *wait_list,
				enum proc_state state)
{
	acquire(&pcblock[p->pid]);
	assert(p->state == TASK_RUNNING or p->state == TASK_READY);
	list_add_front(&p->block_list, wait_list);
	p->state = state;
	release(&pcblock[p->pid]);
}

/**
 * @brief Unblock the 1st process that wait on wait_list. Furthermore, this
 * function will return with holding the lock of the process that it unblocks
 * just now
 * @param wait_list
 * @return struct proc_t*
 */
struct proc_t *proc_unblock(struct list_node_t *wait_list)
{
	struct list_node_t *next_node = list_next_then_del(wait_list);
	struct proc_t *p = element_entry(next_node, struct proc_t, block_list);
	acquire(&pcblock[p->pid]);
	assert(p->state == TASK_BLOCK);
	p->state = TASK_READY;
	return p;
}

__always_inline void proc_unblock_all(struct list_node_t *wait_list)
{
	while (!list_empty(wait_list)) {
		struct proc_t *p = proc_unblock(wait_list);
		release(&pcblock[p->pid]);
	}
}

void time_wakeup()
{
	acquire(&sleep_queue.sleep_lock);
	while (!priority_queue_empty(&sleep_queue.pqm)) {
		struct pair_t temppair = priority_queue_top(&sleep_queue.pqm);
		if (ticks < temppair.key)
			break;
		acquire(&pcblock[temppair.value]);
		assert(pcbtable[temppair.value]->state == TASK_BLOCK);
		pcbtable[temppair.value]->state = TASK_READY;
		release(&pcblock[temppair.value]);
		priority_queue_pop(&sleep_queue.pqm);
	}
	release(&sleep_queue.sleep_lock);
}

int8_t killed(struct proc_t *p)
{
	int8_t k;

	acquire(&pcblock[p->pid]);
	k = p->killed;
	release(&pcblock[p->pid]);
	return k;
}

void recycle_exitedproc() {}

/**
 * @brief a user program that calls fork() then execve() to invoke a shell to
 * interact with I/O which is compiled and assembled from
 * ${cwd}/user/src/initcode.S, and the hexdata dump command is:
 * od user/bin/initcode.o -t x -v
 */
uint32_t initcode[] = {
	0x00b00893, 0x00000517, 0x02c50513, 0x00000597, 0x04058593, 0x00000617,
	0x06960613, 0x00000073, 0x00100893, 0xfff00513, 0x00000073, 0x00000000,
	0x6e69622f, 0x6863652f, 0x6568006f, 0x006f6c6c, 0x6b696e75, 0x6f770073,
	0x00646c72, 0x0001003a, 0x00000000, 0x00010040, 0x00000000, 0x00010046,
	0x00000000, 0x00000000, 0x00000000, 0x454d4f48, 0x50002f3d, 0x3d485441,
	0x6e69622f, 0x01006c00, 0x00000000, 0x01007300, 0x00000000, 0x00000000,
	0x00000000, 0x00000000,
};

void user_init(uint32_t priority)
{
	struct proc_t *p = allocproc();
	initproc = p;

	// allocate 1 page and copy initcode's instructions and data into it
	uvmfirst(p->pagetable, initcode, sizeof(initcode));
	p->sz = PGSIZE;

	/**
	 * @brief forgery context: prepare for the 1st time return from kernel
	 * to user user code start executing at addr USER_TEXT_START
	 */
	p->tf->epc = USER_TEXT_START;
	// initcode will not do any stack operation
	set_proc_name(initproc, "initcode");
	p->parentpid = 0;
	p->state = TASK_READY;

	p->ticks = p->priority = priority;

	release(&pcblock[p->pid]);
}

/* === process relative syscall === */

int64_t do_fork()
{
	struct proc_t *parentproc = myproc(), *childproc = NULL;
	if ((childproc = allocproc()) == NULL) {
		return -1;
	}

	// copy user memory from parent to child with COW mechanism
	if (uvmcopy(parentproc->pagetable, childproc->pagetable,
		    parentproc->sz) < 0) {
		freeproc(childproc->pid);
		release(&pcblock[childproc->pid]);
		return -1;
	}
	childproc->sz = parentproc->sz;

	*(childproc->tf) = *(parentproc->tf);	// copy saved user's registers

	// childproc's ret val of fork need to be set to 0 according to POSIX
	childproc->tf->a0 = 0;

	set_proc_name(childproc, parentproc->name);

	childproc->parentpid = parentproc->pid;
	childproc->state = TASK_READY;

	childproc->jiffies = parentproc->jiffies;
	childproc->ticks = parentproc->ticks;
	childproc->priority = parentproc->priority;

	release(&pcblock[childproc->pid]);
	assert(parentproc->magic == UNIKS_MAGIC);

	return childproc->pid;
}

/**
 * @brief Exit the current process. Does not return. An exited process remains
 * in the zombie state until its parent calls waitpid(). Futhermore, according
 * to the POSIX standard, the exit syscall will not release variety of resources
 * it had retained until another process wait for this process.
 * @param status process exit status
 */
void do_exit(int32_t status)
{
	struct proc_t *p = myproc();

	acquire(&pcblock[p->pid]);

	p->exitstate = status;
	p->state = TASK_ZOMBIE;

	release(&pcblock[p->pid]);

	/**
	 * @brief when wakeup the processes which are waiting for myproc() to
	 * exit, there is a need to acquire more than 1 lock @ the same time, so
	 * we hold an order as how we do when block a process in
	 * sysproc.c:sys_waitpid() function. Now, there is no deadlock
	 * phenomenon between block-function in sysproc.c:sys_waitpid() and this
	 * wakeup-function occurred, I'm not sure that if it's okay to avoid it,
	 * but I hope that's bug free.
	 */
	acquire(&pcblock[p->pid]);
	while (!list_empty(&p->wait_list)) {
		struct proc_t *waitp = proc_unblock(&p->wait_list);
		int64_t *status_addr = (int64_t *)argufetch(waitp, 1);
		verify_area(status_addr, sizeof(pcbtable[p->pid]->exitstate));
		copyout(waitp->pagetable, (void *)status_addr,
			(void *)&pcbtable[p->pid]->exitstate,
			sizeof(pcbtable[p->pid]->exitstate));
		release(&pcblock[waitp->pid]);
	}
	release(&pcblock[p->pid]);

	assert(p->magic == UNIKS_MAGIC);
	sched();
	panic("zombie exit!");
}