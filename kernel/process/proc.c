#include "proc.h"
#include <kassert.h>
#include <kstring.h>
#include <mm/memlay.h>
#include <platform/riscv.h>
#include <queue.h>
#include <sync/spinlock.h>


struct proc *pcbtable[NPROC] = {NULL};
struct proc idlepcb;
struct spinlock pcblock[NPROC];
struct cpu cpus[NCPU];

struct {
	struct spinlock pid_lock;
	struct queue_meta qm;
	int32_t pids_queue_array[NPROC];
} pids_queue;

struct proc *initproc = NULL;	// init proc

extern char trampoline[];
extern void *kalloc();
extern void usertrapret();
extern int32_t mappages(pagetable_t pagetable, uintptr_t va, uintptr_t size,
			uintptr_t pa, int32_t perm, int8_t recordref);
extern void switch_to(struct context *, struct context *);
extern void uvmfirst(pagetable_t, uint32_t *, uint32_t);
extern pagetable_t uvmcreate();
extern void uvmunmap(pagetable_t, uint64_t, uint64_t, int32_t);
extern void uvmfree(pagetable_t, uint64_t), kfree(void *);
extern int64_t uvmcopy(pagetable_t old, pagetable_t new, uint64_t sz);

/**
 * @brief Set the proc->name as argument name with the constraint of max length,
 * and it supposed that the caller will pass a legal string with '\0' terminator
 * @param proc
 * @param name
 */
static void set_proc_name(struct proc *proc, const char *name)
{
	strncpy(proc->name, name, PROC_NAME_LEN);
}

struct cpu *mycpu()
{
	int32_t id = r_mhartid();
	struct cpu *c = &cpus[id];
	return c;
}
__always_inline struct proc *myproc()
{
	return mycpu()->proc;
}

int32_t allocpid()
{
	int32_t pid;
	acquire(&pids_queue.pid_lock);
	pid = *queue_front(&pids_queue.qm);
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
pagetable_t proc_makebasicpagetable(struct proc *p)
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
static void freeproc(int32_t pid)
{
	assert(holding(&pcblock[pid]));
	if (pcbtable[pid]->pagetable)
		proc_freepagetable(pcbtable[pid]->pagetable, pcbtable[pid]->sz);
	kfree(pcbtable[pid]->tf);
	pcbtable[pid] = NULL;
}

/**
 * @brief allocate a new process and fill the tiny context and return with
 * holding the lock of the new process if allocate a process successfully
 *
 * @return struct proc *: new allocate process's pcb entry in pcb table
 */
struct proc *allocproc()
{
	int32_t newpid = allocpid();
	if (newpid != -1)
		goto found;
	return NULL;
found:
	acquire(&pcblock[newpid]);
	assert(pcbtable[newpid] == NULL);

	struct trapframe *tf;
	uintptr_t kstack;

	/**
	 * @brief allocate a kstack page as well as a trapframe page locate at
	 * the begining of kstack
	 */
	if ((tf = kalloc()) == NULL) {
		// if there are no enough memory, failed
		release(&pcblock[newpid]);
		return NULL;
	}
	kstack = (uintptr_t)tf;
	pcbtable[newpid] = (struct proc *)(kstack + sizeof(struct trapframe));
	pcbtable[newpid]->state = TASK_INITING;
	pcbtable[newpid]->pid = newpid;
	pcbtable[newpid]->kstack = kstack + PGSIZE;
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
	// note: still holding p->lock from scheduler
	release(&pcblock[myproc()->pid]);
	usertrapret();
}

__always_inline void initidleproc()
{
	pcbtable[0] = &idlepcb;
	pcbtable[0]->pid = 0;
	pcbtable[0]->kstack = (uintptr_t)&bootstacktop0;
	strcpy(pcbtable[0]->name, "idleproc");
	pcbtable[0]->magic = UNIKS_MAGIC;
	mycpu()->proc = pcbtable[0];
}

// initialize the pcb table lock
void proc_init()
{
	initlock(&pids_queue.pid_lock, "nextpid");
	queue_init(&pids_queue.qm, NPROC, pids_queue.pids_queue_array);
	for (int32_t i = 1; i < NPROC; i++)
		queue_push(&pids_queue.qm, i);
	for (struct spinlock *plk = pcblock; plk < &pcblock[NPROC]; plk++) {
		initlock(plk, "proc");
	}
	initidleproc();
}

// each hart will hold its local scheduler context
void scheduler()
{
	struct cpu *c = mycpu();
	c->proc = pcbtable[0];
	while (1) {
		int32_t i = 1;
		for (struct proc *p; i < NPROC; i++) {
			p = pcbtable[i];
			if (p == NULL)
				continue;
			acquire(&pcblock[p->pid]);
			if (p->state == TASK_READY) {
				tracef("switch to: %d\n", p->pid);
				/**
				 * @brief switch to chosen process. it is the
				 * process's job to release its lock and then
				 * reacquire it before jumping back to us (in
				 * yield())
				 */
				c->proc = p;
				p->state = TASK_RUNNING;
				interrupt_off();
				switch_to(&c->ctxt, &p->ctxt);
				interrupt_on();
				/**
				 * @brief process is done running for now since
				 * timer interrupt
				 */
				c->proc = pcbtable[0];
			}
			release(&pcblock[p->pid]);
		}
	}
}

// switch to kernel thread scheduler and must hold p->lock when entering here
void sched()
{
	struct proc *p = myproc();
	assert(holding(&pcblock[p->pid]));

	p->state = TASK_READY;
	interrupt_off();
	switch_to(&p->ctxt, &mycpu()->ctxt);
	interrupt_on();
}

/**
 * @brief give up the CPU who called this
 */
void yield()
{
	struct proc *p = myproc();
	acquire(&pcblock[p->pid]);
	sched();
	release(&pcblock[p->pid]);
}

/**
 * @brief make process p blocked and mount it to blocked_list list
 *
 * @param p
 * @param list
 * @param state
 */
void proc_block(struct proc *p, struct list_node *list, enum proc_state state)
{
	acquire(&pcblock[p->pid]);
	/**
	 * @brief process can be block only when it hasn't been blocked!
	 */
	assert(list_empty(&p->block_list));
	list_add_front(&p->block_list, list);
	p->state = state;
	if (myproc() == p) {
		sched();
	}
	release(&pcblock[p->pid]);
}

/**
 * @brief unblock process p and remove it from its original block list
 *
 * @param p
 */
void proc_unblock(struct proc *p)
{
	acquire(&pcblock[p->pid]);
	assert(!list_empty(&p->block_list));
	list_del(&p->block_list);
	p->state = TASK_READY;
	release(&pcblock[p->pid]);
}

/**
 * @brief atomically release lock and sleep on sleeplist.
 * reacquires lock when awakened.
 * @param sleeplist
 * @param lk
 */
void sleep(void *sleeplist, struct spinlock *lk)
{
	struct proc *p = myproc();

	/**
	 * @brief must acquire p->lock in order to change p->state and then call
	 * sched. once we hold p->lock, we can be guaranteed that we won't miss
	 * any wakeup (wakeup locks p->lock), so it's okay to release lk.
	 */
	acquire(&pcblock[p->pid]);
	release(lk);

	// go to sleep
	p->sleeplist = sleeplist;
	p->state = TASK_BLOCK;

	sched();

	// tidy up
	p->sleeplist = NULL;

	// reacquire original lock
	release(&pcblock[p->pid]);
	acquire(lk);
}

/**
 * @brief wake up all processes sleeping on sleeplist.
 * must be called without any p->lock
 * @param sleeplist the sleep list that process waiting on
 */
void wakeup(void *sleeplist)
{
	int32_t i = 1;
	for (struct proc *p = pcbtable[i]; i < NPROC; i++) {
		p = pcbtable[i];
		if (p != NULL and p != myproc()) {   // can not wakeup self
			acquire(&pcblock[p->pid]);
			if (p->state == TASK_BLOCK and
			    p->sleeplist == sleeplist) {
				p->state = TASK_READY;
			}
			release(&pcblock[p->pid]);
		}
	}
}

void time_wakeup() {}

int8_t killed(struct proc *p)
{
	int8_t k;

	acquire(&pcblock[p->pid]);
	k = p->killed;
	release(&pcblock[p->pid]);
	return k;
}

/**
 * @brief a user program that calls fork() then write() which is compiled and
 * assembled from ${cwd}/user/src/forktest.c
 * hexdata dump command: od user/bin/forktest -t x
 */
uint32_t initcode[] = {
	0xff010113, 0x00113423, 0x00813023, 0x01010413, 0x008000ef, 0x0000006f,
	0xfe010113, 0x00113c23, 0x00813823, 0x02010413, 0xfe042623, 0x01c0006f,
	0x12c000ef, 0x00050793, 0x02078263, 0xfec42783, 0x0017879b, 0xfef42623,
	0xfec42783, 0x0007871b, 0x00600793, 0xfce7dee3, 0x0080006f, 0x00000013,
	0x130000ef, 0x00050793, 0x0ff7f793, 0x0307879b, 0x0ff7f713, 0x21000793,
	0x00e784a3, 0x00b00593, 0x21000513, 0x140000ef, 0xff5ff06f, 0xf9010113,
	0x02813423, 0x03010413, 0xfca43c23, 0x00b43423, 0x00c43823, 0x00d43c23,
	0x02e43023, 0x02f43423, 0x03043823, 0x03143c23, 0x04040793, 0xfcf43823,
	0xfd043783, 0xfc878793, 0xfef43423, 0xfe843783, 0x00878713, 0xfee43423,
	0x0007b783, 0x00078513, 0xfe843783, 0x00878713, 0xfee43423, 0x0007b783,
	0x00078593, 0xfe843783, 0x00878713, 0xfee43423, 0x0007b783, 0x00078613,
	0xfe843783, 0x00878713, 0xfee43423, 0x0007b783, 0x00078693, 0xfe843783,
	0x00878713, 0xfee43423, 0x0007b783, 0x00078713, 0xfe843783, 0x00878813,
	0xff043423, 0x0007b783, 0xfd843883, 0x00000073, 0x00050793, 0x00078513,
	0x02813403, 0x07010113, 0x00008067, 0xff010113, 0x00113423, 0x00813023,
	0x01010413, 0x00100513, 0xf1dff0ef, 0x00050793, 0x0007879b, 0x00078513,
	0x00813083, 0x00013403, 0x01010113, 0x00008067, 0xff010113, 0x00113423,
	0x00813023, 0x01010413, 0x00b00513, 0xee9ff0ef, 0x00050793, 0x0007879b,
	0x00078513, 0x00813083, 0x00013403, 0x01010113, 0x00008067, 0xfe010113,
	0x00113c23, 0x00813823, 0x02010413, 0xfea43423, 0xfeb43023, 0xfe043683,
	0xfe843603, 0x00000593, 0x01000513, 0xea1ff0ef, 0x00050793, 0x0007879b,
	0x00078513, 0x01813083, 0x01013403, 0x02010113, 0x00008067, 0x00000000,
	0x74696e69, 0x636f7270, 0x00093120, 0x00000000,
};

void user_init(uint32_t priority)
{
	struct proc *p = allocproc();
	initproc = p;

	// allocate 1 page and copy initcode's instructions and data into it
	uvmfirst(p->pagetable, initcode, sizeof(initcode));
	p->sz = PGSIZE;

	// forgery context: prepare for the 1st time return from kernel to user
	p->tf->epc = 0;	      // user code start executing at addr 0x0
	p->tf->sp = PGSIZE;   // user stack pointer
	set_proc_name(initproc, "initcode");
	p->parent = NULL;
	p->state = TASK_READY;

	p->ticks = p->priority = priority;

	release(&pcblock[p->pid]);
}

// process relative syscall
int64_t do_fork()
{
	struct proc *parentproc = myproc(), *childproc = NULL;
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

	childproc->parent = parentproc;
	childproc->state = TASK_READY;

	childproc->jiffies = parentproc->jiffies;
	childproc->ticks = parentproc->ticks;
	childproc->priority = parentproc->priority;
	release(&pcblock[childproc->pid]);
	assert(myproc()->magic == UNIKS_MAGIC);
	return childproc->pid;
}

int64_t do_exec()
{
	kprintf("do exec syscall\n");
	return 0;
}