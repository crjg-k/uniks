#include "proc.h"
#include <device/clock.h>
#include <file/file.h>
#include <fs/ext2fs.h>
#include <loader/elfloader.h>
#include <mm/memlay.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <platform/platform.h>
#include <platform/riscv.h>
#include <sys/ksyscall.h>
#include <trap/trap.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>
#include <uniks/log.h>
#include <uniks/param.h>


struct proc_t *pcbtable[NPROC] = {NULL};
struct proc_t idlepcb = {
	.pid = 0,
	.parentpid = 0,
	.host = NULL,
	.state = TASK_RUNNING,
	.killed = 0,
	.exitstate = 0,
	.priority = 0,
	.ticks = 0,
	.jiffies = 0,
	.block_list = {},
	.wait_list = {},
	.cwd = NULL,
	.icwd = NULL,
	.fdtable = {0},
	.mm = NULL,
	.ctxt = {},
	.tf = NULL,
	.name = "idleproc",
	.magic = UNIKS_MAGIC,
};
struct spinlock_t pcblock[NPROC];
/**
 * @brief Helps ensure that wakeups of wait()ing parents are not lost. Helps
 * obey the memory model when using p->parent. Must be acquired before any
 * p->lock.
 */
struct spinlock_t wait_lock;
struct cpu_t cpus[MAXNUM_HARTID];

struct pids_queue_t pids_queue;
struct sleep_queue_t sleep_queue;   // call sys_sleep waiting process

extern char trampoline[];
extern int64_t do_execve(struct proc_t *p, char *pathname, char *argv[],
			 char *envp[]);


__always_inline struct cpu_t *mycpu()
{
	return (struct cpu_t *)read_csr(sscratch);
}

__always_inline struct proc_t *myproc()
{
	push_off();
	struct proc_t *p = mycpu()->proc;
	pop_off();
	return p;
}

static pid_t allocpid()
{
	pid_t pid;
	acquire(&pids_queue.pid_lock);
	if (queue_empty(&pids_queue.qm))
		pid = -1;
	else {
		pid = *(pid_t *)queue_front_int32type(&pids_queue.qm);
		queue_front_pop(&pids_queue.qm);
	}
	release(&pids_queue.pid_lock);
	return pid;
}

void freepid(pid_t pid)
{
	acquire(&pids_queue.pid_lock);
	queue_push_int32type(&pids_queue.qm, pid);
	release(&pids_queue.pid_lock);
}

/**
 * @brief Create a basic user page table for a given mm_struct, and only initial
 * with trampoline and trapframe pages.
 * @param p
 * @return int32_t
 */
int32_t user_basic_pagetable(struct proc_t *p)
{
	/**
	 * @brief map the trampoline.S code at the highest user virtual address.
	 * only S mode can use it, on the way to/from user space, so not PTE_U
	 */
	if (mappages(p->mm->pagetable, TRAMPOLINE, PGSIZE,
		     (uintptr_t)trampoline, PTE_R | PTE_X) == -1)
		return -1;
	// map the trapframe page just below the trampoline page
	if (mappages(p->mm->pagetable, TRAPFRAME, PGSIZE, (uintptr_t)(p->tf),
		     PTE_R | PTE_W) == -1)
		return -1;

	return 0;
}

/**
 * @brief free kstack of a proc which contains the proc structure represents it
 * and also include user pages (unmap then free physical memory), then release
 * the corresponding pointer in pcbtable.
 * p->lock must be held before calling this function
 * @param p
 */
static void freeproc(struct proc_t *p)
{
	assert(holding(&pcblock[p->pid]));

	// Close all open files.
	for (int32_t fd = 0; fd < NFD; fd++) {
		if (p->fdtable[fd] != -1) {
			file_close(p->fdtable[fd]);
			p->fdtable[fd] = -1;
		}
	}

	iput(p->icwd);
	kfree(p->cwd);

	if (p->mm->pagetable)
		free_pgtable(p->mm->pagetable, 0);
	free_mm_struct(p->mm);
	kfree(p->name);
}

// a fork child's 1st scheduling by scheduler() will swtch to forkret
void forkret()
{
	static int32_t first = 1;

	struct proc_t *p = myproc();

	// Still holding p->lock from scheduler.
	release(&pcblock[p->pid]);

	if (first) {
		/**
		 * @brief FS initialization must be run in the context of a
		 * regular process (e.g., because it calls proc_block), and thus
		 * cannot be run @kernel_start.
		 */
		first = 0;
		ext2fs_init(VIRTIO_IRQ);
		p->icwd = namei(ROOTPATH, 0);
		assert((p->cwd = kmalloc(PATH_MAX)) != NULL);
		strcpy(p->cwd, ROOTPATH);
	}

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
	if ((tf = pages_alloc(1)) == NULL)
		goto err;
	struct proc_t *p = pcbtable[newpid] =
		(struct proc_t *)((uintptr_t)tf + sizeof(struct trapframe_t));
	p->pid = newpid;
	if ((p->mm = new_mm_struct()) == NULL) {
		pages_free(tf);
		goto err;
	}
	tf->ra = 0;
	p->tf = tf;

	if (user_basic_pagetable(p) == -1) {
		pages_free(tf);
		free_mm_struct(p->mm);
		goto err;
	}

	p->state = TASK_INITING;
	p->killed = p->w4child = 0;
	p->mm->kstack = (uintptr_t)tf + PGSIZE;

	/**
	 * @brief set new ctxt to start executing at forkret, where returns to
	 * user space. this forges a trap scene for new forked process to
	 * release lock right set sp point to the correct kstack address
	 */
	memset(&p->ctxt, 0, sizeof(p->ctxt));
	p->ctxt.ra = (uint64_t)forkret;
	p->ctxt.sp = p->mm->kstack;
	p->magic = UNIKS_MAGIC;

	INIT_LIST_HEAD(&p->block_list);
	INIT_LIST_HEAD(&p->wait_list);
	INIT_LIST_HEAD(&p->child_list);
	INIT_LIST_HEAD(&p->parentp);

	return p;

err:
	release(&pcblock[newpid]);
	freepid(newpid);
	return NULL;
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

	FIRST_PROC = &idlepcb;

	initlock(&sleep_queue.sleep_lock, "sleeplock");
	initlock(&wait_lock, "waitlock");
	priority_queue_init(&sleep_queue.pqm, NPROC,
			    &sleep_queue.sleep_queue_array);
}

// each hart will hold its local scheduler context
__noreturn void scheduler(struct cpu_t *c)
{
	while (1) {
		int32_t i = 1;
		for (struct proc_t *p; i < NPROC; i++) {
			acquire(&pcblock[i]);
			if ((p = pcbtable[i]) == NULL) {
				release(&pcblock[i]);
				continue;
			}
			// tracef("idle process running");
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
				p->host = c;
				switch_to(&c->ctxt, &p->ctxt);
				/**
				 * @brief process is done running for now since
				 * timer interrupt
				 */
				c->proc = FIRST_PROC;
			}
			release(&pcblock[p->pid]);
		}
	}
}

__naked void switch_to(struct context_t *old, struct context_t *new)
{
	asm volatile("sd ra, 0(a0)\n\t"
		     "sd sp, 8(a0)\n\t"
		     "sd s0, 16(a0)\n\t"
		     "sd s1, 24(a0)\n\t"
		     "sd s2, 32(a0)\n\t"
		     "sd s3, 40(a0)\n\t"
		     "sd s4, 48(a0)\n\t"
		     "sd s5, 56(a0)\n\t"
		     "sd s6, 64(a0)\n\t"
		     "sd s7, 72(a0)\n\t"
		     "sd s8, 80(a0)\n\t"
		     "sd s9, 88(a0)\n\t"
		     "sd s10, 96(a0)\n\t"
		     "sd s11, 104(a0)"
		     :
		     :
		     : "memory");
	asm volatile("ld ra, 0(a1)\n\t"
		     "ld sp, 8(a1)\n\t"
		     "ld s0, 16(a1)\n\t"
		     "ld s1, 24(a1)\n\t"
		     "ld s2, 32(a1)\n\t"
		     "ld s3, 40(a1)\n\t"
		     "ld s4, 48(a1)\n\t"
		     "ld s5, 56(a1)\n\t"
		     "ld s6, 64(a1)\n\t"
		     "ld s7, 72(a1)\n\t"
		     "ld s8, 80(a1)\n\t"
		     "ld s9, 88(a1)\n\t"
		     "ld s10, 96(a1)\n\t"
		     "ld s11, 104(a1)");
	asm volatile("ret");
}

/**
 * @brief Switch to scheduler. Must hold only proc->lock (because NO process can
 * hold any spin lock then becomes BLOCK state) and have changed proc->state.
 * Saves and restores intstat because intstat is a property of this kernel
 * thread, not this CPU (In multi-hart condition, when a process exit executing
 * in a hart, it may re-execute in another hart, so this is the situation that
 * we need to tackle).
 */
void sched()
{
	uint64_t preintstat;
	struct proc_t *p = myproc();

	assert(holding(&pcblock[p->pid]));
	assert(mycpu()->repeat == 1);
	assert(p->state != TASK_RUNNING);
	assert(!get_var_bit(interrupt_get(), SSTATUS_SIE));

	preintstat = mycpu()->preintstat;
	switch_to(&p->ctxt, &mycpu()->ctxt);
	// may change host hart when comes back
	mycpu()->preintstat = preintstat;
}

// Proactively give up the CPU who called this.
void yield()
{
	struct proc_t *p = myproc();
	assert(holding(&pcblock[p->pid]));
	p->state = TASK_READY;
	sched();
	release(&pcblock[p->pid]);
}

/**
 * @brief Make myproc() blocked and mount it to wait_list. Furthermore, the lk
 * is supposed to be held before call this function. This function originate
 * from project xv6-riscv/kernel/proc.c:536:sleep().
 *
 * @param wait_list
 * @param lk
 */
void proc_block(struct list_node_t *wait_list, struct spinlock_t *lk)
{
	struct proc_t *p = myproc();
	assert(p->state == TASK_RUNNING);

	/**
	 * @brief Must acquire p->lock in order to change p->state and then
	 * call sched().
	 */
	acquire(&pcblock[p->pid]);

	list_add_front(&p->block_list, wait_list);
	p->state = TASK_BLOCK;

	if (lk != NULL)
		release(lk);

	sched();

	// reacquire original lock
	release(&pcblock[p->pid]);
	if (lk != NULL)
		acquire(lk);
}

/**
 * @brief Unblock all processes blocked on wait_list. Must be called with lock
 * protecting for wait_list.
 * @param wait_list
 * @param lk
 * @return int32_t:
 */
int32_t proc_unblock_all(struct list_node_t *wait_list)
{
	int32_t tot = 0;
	while (!list_empty(wait_list)) {
		struct list_node_t *next_node = list_next_then_del(wait_list);
		struct proc_t *p =
			element_entry(next_node, struct proc_t, block_list);

		acquire(&pcblock[p->pid]);
		assert(p->state == TASK_BLOCK);
		assert(p != myproc());
		p->state = TASK_READY;
		release(&pcblock[p->pid]);
		tot++;
	}

	return tot;
}

void time_wakeup()
{
	acquire(&sleep_queue.sleep_lock);
	while (!priority_queue_empty(&sleep_queue.pqm)) {
		struct pair_t temppair = priority_queue_top(&sleep_queue.pqm);
		if (atomic_load(&ticks) < temppair.key)
			break;
		struct proc_t *p = pcbtable[temppair.value];

		acquire(&pcblock[p->pid]);
		assert(p->state == TASK_BLOCK);
		assert(p != myproc());
		p->state = TASK_READY;
		release(&pcblock[p->pid]);

		priority_queue_pop(&sleep_queue.pqm);
	}
	release(&sleep_queue.sleep_lock);
}

void setkilled(struct proc_t *p)
{
	acquire(&pcblock[p->pid]);
	p->killed = 1;
	release(&pcblock[p->pid]);
}

int32_t killed(struct proc_t *p)
{
	int32_t k;

	acquire(&pcblock[p->pid]);
	k = p->killed;
	release(&pcblock[p->pid]);
	return k;
}

/**
 * @brief The 1st user program that calls execve("/bin/initrc"). Assembled from
 * ../user/src/initcode.S.
 * Command: `hexdump -ve '4/4 "0x%08x, " "\n"' ./user/bin/initcode`
 */
uint32_t initcode[] = {
	0x03b00893, 0x00000517, 0x02c50513, 0x00000597, 0x04458593, 0x00000617,
	0x08c60613, 0x00000073, 0x03c00893, 0xfff00513, 0x00000073, 0x00000000,
	0x6e69622f, 0x696e692f, 0x00637274, 0x6c6c6568, 0x6e75006f, 0x00736b69,
	0x6c726f77, 0x00000064, 0x00001030, 0x00000000, 0x0000103c, 0x00000000,
	0x00001042, 0x00000000, 0x00001048, 0x00000000, 0x00000000, 0x00000000,
	0x48544150, 0x69622f3d, 0x4f48006e, 0x2f3d454d, 0x746f6f72, 0x45485300,
	0x2f3d4c4c, 0x2f6e6962, 0x0068736b, 0x00000000, 0x00001078, 0x00000000,
	0x00001082, 0x00000000, 0x0000108d, 0x00000000, 0x00000000, 0x00000000,
};

void user_init(uint32_t priority)
{
	struct proc_t *p = allocproc();
	assert(p != NULL);
	assert(p->pid == 1);
	p->parentpid = 0;
	p->state = TASK_READY;
	p->ticks = p->priority = priority;
	p->name = "initrc";

	for (int32_t fd = 0; fd < NFD; fd++)
		p->fdtable[fd] = -1;
	release(&pcblock[p->pid]);

// this value is corresponding to user/user.ld
#define INITSTART 0x1000
	char *initpage = pages_alloc(1);
	assert(initpage != NULL);
	assert(mappages(p->mm->pagetable, INITSTART, PGSIZE,
			(uintptr_t)initpage, PTE_X | PTE_R | PTE_U) != -1);
	add_vm_area(p->mm, INITSTART, INITSTART + PGSIZE, PTE_X | PTE_R | PTE_U,
		    0, NULL, 0);
	memcpy(initpage, initcode, sizeof(initcode));
	p->tf->epc = INITSTART;
}


/* === process relative syscall === */

int64_t do_fork()
{
	struct proc_t *parentproc = myproc(), *childproc = NULL;
	if ((childproc = allocproc()) == NULL)
		return -1;

	// copy user memory from parent to child with COW mechanism
	uvm_space_copy(childproc->mm, parentproc->mm);

	*(childproc->tf) = *(parentproc->tf);	// copy saved user's registers

	// childproc's ret val of fork need to be set to 0 according to POSIX
	childproc->tf->a0 = 0;

	// increment reference counts on open file descriptors
	for (int32_t i = 0; i < NFD; i++)
		childproc->fdtable[i] = file_dup(parentproc->fdtable[i]);
	childproc->icwd = idup(parentproc->icwd);
	childproc->cwd = kmalloc(PATH_MAX);
	strcpy(childproc->cwd, parentproc->cwd);

	acquire(&wait_lock);
	childproc->parentpid = parentproc->pid;
	list_add_front(&childproc->parentp, &parentproc->child_list);
	release(&wait_lock);

	childproc->state = TASK_READY;
	childproc->jiffies = parentproc->jiffies;
	childproc->ticks = parentproc->ticks;
	childproc->priority = parentproc->priority;

	childproc->name = kmalloc(EXT2_NAME_LEN);
	strcpy(childproc->name, parentproc->name);

	release(&pcblock[childproc->pid]);
	assert(parentproc->magic == UNIKS_MAGIC);
	assert(childproc->magic == UNIKS_MAGIC);

	return childproc->pid;
}

// Pass p's abandoned children to INIT_PROC. Caller must hold wait_lock.
void reparent(struct proc_t *p)
{
	while (!list_empty(&p->child_list)) {
		struct list_node_t *childn = list_next_then_del(&p->child_list);
		struct proc_t *childp =
			element_entry(childn, struct proc_t, parentp);
		assert(childp->parentpid == p->pid);

		acquire(&pcblock[INIT_PROC->pid]);
		acquire(&pcblock[childp->pid]);
		list_add_front(childn, &INIT_PROC->child_list);
		childp->parentpid = INIT_PROC->pid;
		release(&pcblock[childp->pid]);
		if (INIT_PROC->w4child == 1) {
			assert(INIT_PROC->state = TASK_BLOCK);
			INIT_PROC->state = TASK_READY;
		}
		release(&pcblock[INIT_PROC->pid]);
	}
}

/**
 * @brief Exit the current process. Does not return. An exited process
 * remains in the zombie state until its parent calls waitpid().
 * Futhermore, according to the POSIX standard, the exit syscall will
 * not release variety of resources it had retained until another
 * process wait for this process.
 * @param status process exit status
 */
void do_exit(int32_t status)
{
	struct proc_t *p = myproc();
	assert(p != FIRST_PROC);
	assert(p != INIT_PROC);

	acquire(&wait_lock);
	// Give any childproc to INIT_PROC.
	reparent(p);

	acquire(&pcblock[p->pid]);
	p->exitstate = status;
	p->state = TASK_ZOMBIE;

	/**
	 * @brief free all memorys and files that the process holds, including
	 * the kstack, the pagetable, physical memory page which is
	 * mapped by pagetable and etc.
	 */
	freeproc(p);

	/**
	 * @brief Unblock all the processes which are waiting for
	 * myproc() to exit.
	 */
	int32_t num = proc_unblock_all(&p->wait_list);
	if (num == 0) {
		/**
		 * @brief Means that no other process is waiting for the process
		 * to exit through waitpid(). So Next, it is necessary to check
		 * whether the parent process is waiting through wait().
		 */
		struct proc_t *parentp = pcbtable[p->parentpid];
		acquire(&pcblock[parentp->pid]);
		assert(p->parentpid == parentp->pid);
		if (parentp->w4child == 1) {
			assert(parentp->state = TASK_BLOCK);
			parentp->state = TASK_READY;
		}
		release(&pcblock[parentp->pid]);
	}

	release(&wait_lock);
	// Jump into the scheduler, never to return.
	sched();
	BUG();
}