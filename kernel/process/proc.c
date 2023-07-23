#include "proc.h"
#include <loader/elfloader.h>
#include <mm/memlay.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <platform/riscv.h>
#include <sync/spinlock.h>
#include <sys/ksyscall.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>
#include <uniks/list.h>
#include <uniks/param.h>


struct proc_t *pcbtable[NPROC] = {NULL};
struct proc_t idlepcb = {
	.pid = 0,
	.parentpid = 0,
	.state = TASK_READY,
	.killed = 0,
	.exitstate = 0,
	.priority = 0,
	.ticks = 0,
	.jiffies = 0,
	.block_list = {},
	.wait_list = {},
	.fdtable = {0},
	.mm = NULL,
	.ctxt = {},
	.tf = NULL,
	.name = "idleproc",
	.magic = UNIKS_MAGIC,
};
struct spinlock_t pcblock[NPROC];
struct cpu_t cpus[NCPU];

struct pids_queue_t pids_queue;
struct sleep_queue_t sleep_queue;   // call sys_sleep waiting process

extern char trampoline[];
extern volatile uint64_t ticks;
extern void usertrapret();
extern void switch_to(struct context_t *old, struct context_t *new);
extern int64_t do_execve(struct proc_t *p, char *pathname, char *argv[],
			 char *envp[]);

/**
 * @brief Set the proc->name as argument name with the constraint of max length,
 * and it supposed that the caller will pass a legal string with '\0' terminator
 * @param proc
 * @param name
 */
__always_inline static void set_proc_name(struct proc_t *proc, const char *name)
{
	strncpy(proc->name, name, FILENAMELEN);
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

static int32_t allocpid()
{
	pid_t pid;
	acquire(&pids_queue.pid_lock);
	if (queue_empty(&pids_queue.qm))
		pid = -1;
	else {
		pid = *(pid_t *)queue_front_int32type(&pids_queue.qm);
		queue_pop(&pids_queue.qm);
	}
	release(&pids_queue.pid_lock);
	return pid;
}

static void freepid(pid_t pid)
{
	acquire(&pids_queue.pid_lock);
	queue_push_int32type(&pids_queue.qm, pid);
	release(&pids_queue.pid_lock);
}

/**
 * @brief create a basic user page table for a given process, and only initial
 * with trampoline and trapframe pages
 *
 * @param p
 * @return int32_t
 */
int32_t user_basic_pagetable(struct proc_t *p)
{
	/**
	 * @brief map the trampoline.S code at the highest user virtual address.
	 * only S mode can use it, on the way to/from user space, so not PTE_U
	 */
	if (mappages(p->mm->pagetable, TRAMPOLINE, PGSIZE, (uint64_t)trampoline,
		     PTE_R | PTE_X) == -1)
		return -1;
	// map the trapframe page just below the trampoline page
	if (mappages(p->mm->pagetable, TRAPFRAME, PGSIZE, (uint64_t)(p->tf),
		     PTE_R | PTE_W) == -1)
		return -1;

	return 0;
}

// free a process's page table, and free the physical memory it refers to
__always_inline void proc_free_pagetable(struct proc_t *p)
{
	uvmfree(p->mm);
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
	if (pcbtable[pid]->mm->pagetable)
		proc_free_pagetable(pcbtable[pid]);
	pages_free(pcbtable[pid]->tf);
	pcbtable[pid] = NULL;
}

// a fork child's 1st scheduling by scheduler() will swtch to forkret
void forkret()
{
	usertrapret();
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
	if (newpid == -1)
		return NULL;

	acquire(&pcblock[newpid]);
	struct trapframe_t *tf;

	/**
	 * @brief allocate a kstack page as well as a trapframe page locate at
	 * the begining of kstack
	 */
	if ((tf = pages_alloc(1, 0)) == NULL)
		goto err;
	pcbtable[newpid] =
		(struct proc_t *)((uintptr_t)tf + sizeof(struct trapframe_t));
	pcbtable[newpid]->pid = newpid;
	pcbtable[newpid]->mm = kmalloc(sizeof(struct mm_struct));
	if (init_mm(pcbtable[newpid]->mm) == -1) {
		pages_free(tf);
		goto err;
	}
	if (user_basic_pagetable(pcbtable[newpid]) == -1) {
		pages_free(tf);
		free_mm(pcbtable[newpid]->mm);
		goto err;
	}

	pcbtable[newpid]->state = TASK_INITING;
	pcbtable[newpid]->killed = 0;
	pcbtable[newpid]->mm->kstack = (uintptr_t)tf + PGSIZE;
	pcbtable[newpid]->tf = tf;
	/**
	 * @brief set new ctxt to start executing at forkret, where returns to
	 * user space. this forges a trap scene for new forked process to
	 * release lock right set sp point to the correct kstack address
	 */
	pcbtable[newpid]->ctxt.ra = (uint64_t)forkret;
	pcbtable[newpid]->ctxt.sp = pcbtable[newpid]->mm->kstack;
	pcbtable[newpid]->magic = UNIKS_MAGIC;

	INIT_LIST_HEAD(&pcbtable[newpid]->block_list);
	INIT_LIST_HEAD(&pcbtable[newpid]->wait_list);

	return pcbtable[newpid];
err:
	release(&pcblock[newpid]);
	freepid(newpid);
	return NULL;
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

// give up the CPU who called this
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

int32_t killed(struct proc_t *p)
{
	int32_t k;

	acquire(&pcblock[p->pid]);
	k = p->killed;
	release(&pcblock[p->pid]);
	return k;
}

void recycle_exitedproc(pid_t pid)
{
	freeproc(pid);
}

void user_init(uint32_t priority)
{
	struct proc_t *p = allocproc();
	assert(p != NULL);
	p->parentpid = 0;
	p->state = TASK_READY;
	p->ticks = p->priority = priority;
	release(&pcblock[p->pid]);

	extern char initcode[];
	assert(do_execve(p, initcode, NULL, NULL) == 0);
}

/* === process relative syscall === */

int64_t do_fork()
{
	struct proc_t *parentproc = myproc(), *childproc = NULL;
	if ((childproc = allocproc()) == NULL) {
		return -1;
	}

	// copy user memory from parent to child with cow mechanism
	if (uvmcopy(parentproc->mm, childproc->mm) < 0) {
		freeproc(childproc->pid);
		release(&pcblock[childproc->pid]);
		return -1;
	}

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
	// acquire(&pcblock[p->pid]);
	// while (!list_empty(&p->wait_list)) {
	// 	struct proc_t *waitp = proc_unblock(&p->wait_list);
	// 	int64_t *status_addr = (int64_t *)argufetch(waitp, 1);
	// 	verify_area(status_addr, sizeof(pcbtable[p->pid]->exitstate));
	// 	copyout(waitp->mm->pagetable, (void *)status_addr,
	// 		(void *)&pcbtable[p->pid]->exitstate,
	// 		sizeof(pcbtable[p->pid]->exitstate));
	// 	release(&pcblock[waitp->pid]);
	// }
	// release(&pcblock[p->pid]);

	assert(p->magic == UNIKS_MAGIC);
	// recycle_exitedproc(p->pid);
	sched();
	panic("zombie exit!");
}