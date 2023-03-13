#include "proc.h"
#include <kassert.h>
#include <kstring.h>
#include <mm/memlay.h>
#include <platform/riscv.h>


struct proc pcbtable[NPROC];
struct cpu cpus[NCPU];
struct spinlock pid_lock;
static volatile int32_t nextpid = 1;
struct proc *initproc = NULL;	// init proc

extern char trampoline[];
extern void *kalloc();
extern void usertrapret();
extern int32_t mappages(pagetable_t pagetable, uintptr_t va, uintptr_t size,
			uintptr_t pa, int32_t perm);
extern void switch_to(struct context *, struct context *);
extern void uvmfirst(pagetable_t, uint32_t *, uint32_t);
extern pagetable_t uvmcreate();
extern void uvmunmap(pagetable_t, uint64_t, uint64_t, int32_t);
extern void uvmfree(pagetable_t, uint64_t), kfree(void *);
extern int64_t uvmcopy(pagetable_t old, pagetable_t new, uint64_t sz);

static void set_proc_name(struct proc *proc, const char *name)
{
	int32_t len = strlen(name);
	assert(len <= PROC_NAME_LEN);
	memcpy(proc->name, name, len);
	proc->name[len] = 0;
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
	acquire(&pid_lock);
	pid = nextpid++;
	release(&pid_lock);
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
		     PTE_R | PTE_X) < 0) {
		uvmfree(pagetable, 0);
		return 0;
	}

	// map the trapframe page just below the trampoline page
	if (mappages(pagetable, TRAPFRAME, PGSIZE, (uint64_t)(p->tf),
		     PTE_R | PTE_W) < 0) {
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
 * @brief allocate a page for each process's kernel stack which
 * followed by an invalid guard page just below it
 *
 * @param kpgtbl kernel page table
 */
void proc_mapstacks(pagetable_t kpgtbl)
{
	uint64_t va, pa;
	for (struct proc *p = pcbtable; p < &pcbtable[NPROC]; p++) {
		pa = (uint64_t)kalloc();
		assert(pa != 0);
		va = KSTACK((int32_t)(p - pcbtable));
		mappages(kpgtbl, va, PGSIZE, pa, PTE_R | PTE_W);
	}
}

/**
 * @brief free a proc structure and the data hanging from it including user
 * pages (unmap then free physical memory). p->lock must be held
 *
 * @param p
 */
static void freeproc(struct proc *p)
{
	assert(holding(&p->lock));
	if (p->tf)
		kfree((void *)p->tf), p->tf = NULL;
	if (p->pagetable)
		proc_freepagetable(p->pagetable, p->sz), p->pagetable = 0;
	p->sz = 0;
	p->name[0] = p->pid = 0;
	p->parent = NULL;
	p->state = UNUSED;
}

/**
 * @brief allocate a new process and fill the tiny context and return with
 * holding the lock of the new process if allocate a process successfully
 *
 * @return struct proc *: new allocate process's pcb entry in pcb table
 */
struct proc *allocproc()
{
	struct proc *p;
	for (p = pcbtable; p < &pcbtable[NPROC]; p++) {
		acquire(&p->lock);
		if (p->state == UNUSED) {
			goto found;
		} else {
			release(&p->lock);
		}
	}
	return NULL;
found:
	p->pid = allocpid();
	p->state = INITING;

	// allocate a trapframe page
	if ((p->tf = (struct trapframe *)kalloc()) == NULL) {
		// if there are no enough memory, failed
		freeproc(p);
		release(&p->lock);
		return NULL;
	}

	// an empty user page table
	if ((p->pagetable = proc_makebasicpagetable(p)) == 0) {
		// if there are no enough memory, failed
		freeproc(p);
		release(&p->lock);
		return NULL;
	}

	/**
	 * @brief set new ctxt to start executing at forkret, where returns to
	 * user space. this forges a trap scene for new forked process to
	 * release lock right set sp point to the correct kstack address
	 */
	memset(&p->ctxt, 0, sizeof(p->ctxt));
	void forkret();
	p->ctxt.ra = (uint64_t)forkret;
	p->ctxt.sp = p->kstack + PGSIZE;

	return p;
}

// a fork child's 1st scheduling by scheduler() will swtch to forkret
void forkret()
{
	// note: still holding p->lock from scheduler
	release(&myproc()->lock);
	usertrapret();
}

// initialize the pcb table
void proc_init()
{
	// init the global pcb table
	initlock(&pid_lock, "nextpid");
	for (struct proc *p = pcbtable; p < &pcbtable[NPROC]; p++) {
		initlock(&p->lock, "proc");
		p->state = UNUSED;
		p->kstack = KSTACK((int32_t)(p - pcbtable));
		p->parent = NULL;
	}
}

// each hart will hold a local scheduler context
void scheduler()
{
	struct proc *p;
	struct cpu *c = mycpu();
	c->proc = NULL;
	while (1) {
		interrupt_on();	  // ensure that the interrupt is on
		for (p = pcbtable; p < &pcbtable[NPROC]; p++) {
			acquire(&p->lock);
			if (p->state == READY) {
				debugf("switch to: %d\n", p - pcbtable);
				// switch to chosen process. it is the process's
				// job to release its lock and then reacquire it
				// before jumping back to us (in yield())
				c->proc = p;
				p->state = RUNNING;
				switch_to(&c->ctxt, &p->ctxt);

				// process is done running for now since timer
				// interrupt
				c->proc = NULL;
			}
			release(&p->lock);
		}
	}
}

// switch to kernel thread scheduler and must hold p->lock when entering here
void sched(void)
{
	struct proc *p = myproc();
	assert(holding(&p->lock));
	
	p->state = READY;
	interrupt_off();
	switch_to(&p->ctxt, &mycpu()->ctxt);
}

// give up the CPU who called this
void yield()
{
	struct proc *p = myproc();
	acquire(&p->lock);

	sched();
	release(&p->lock);
}

/**
 * @brief a user program that calls fork() then write() which is compiled and
 * assembled from ${cwd}/user/src/forktest.c
 * hexdata dump command: od user/bin/forktest -t x
 */
uint32_t initcode[] = {
	0xff010113, 0x00113423, 0x00813023, 0x01010413, 0x008000ef, 0x0000006f,
	0xff010113, 0x00113423, 0x00813023, 0x01010413, 0x128000ef, 0x00050793,
	0x04079063, 0x1d000793, 0x03200713, 0x00e784a3, 0x110000ef, 0x00050793,
	0x02079463, 0x1d000793, 0x03300713, 0x00e784a3, 0x0f8000ef, 0x00050793,
	0x00079863, 0x1d000793, 0x03400713, 0x00e784a3, 0x00b00593, 0x1d000513,
	0x10c000ef, 0xff5ff06f, 0xf9010113, 0x02813423, 0x03010413, 0xfca43c23,
	0x00b43423, 0x00c43823, 0x00d43c23, 0x02e43023, 0x02f43423, 0x03043823,
	0x03143c23, 0x04040793, 0xfcf43823, 0xfd043783, 0xfc878793, 0xfef43423,
	0xfe843783, 0x00878713, 0xfee43423, 0x0007b783, 0x00078513, 0xfe843783,
	0x00878713, 0xfee43423, 0x0007b783, 0x00078593, 0xfe843783, 0x00878713,
	0xfee43423, 0x0007b783, 0x00078613, 0xfe843783, 0x00878713, 0xfee43423,
	0x0007b783, 0x00078693, 0xfe843783, 0x00878713, 0xfee43423, 0x0007b783,
	0x00078713, 0xfe843783, 0x00878813, 0xff043423, 0x0007b783, 0xfd843883,
	0x00000073, 0x00050793, 0x00078513, 0x02813403, 0x07010113, 0x00008067,
	0xff010113, 0x00113423, 0x00813023, 0x01010413, 0x00100513, 0xf1dff0ef,
	0x00050793, 0x0007879b, 0x00078513, 0x00813083, 0x00013403, 0x01010113,
	0x00008067, 0xfe010113, 0x00113c23, 0x00813823, 0x02010413, 0xfea43423,
	0xfeb43023, 0xfe043683, 0xfe843603, 0x00000593, 0x01000513, 0xed5ff0ef,
	0x00050793, 0x0007879b, 0x00078513, 0x01813083, 0x01013403, 0x02010113,
	0x00008067, 0x00000000, 0x74696e69, 0x636f7270, 0x000a3120, 0x00000000,
};

void user_init()
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
	p->state = READY;

	release(&p->lock);
}

int64_t do_fork()
{
	struct proc *parentproc = myproc(), *childproc = NULL;
	if (((childproc) = allocproc()) == 0) {
		return -1;
	}
	// copy user memory from parent to child with COW mechanism
	if (uvmcopy(parentproc->pagetable, childproc->pagetable,
		    parentproc->sz) < 0) {
		freeproc(childproc);
		release(&childproc->lock);
		return -1;
	}
	childproc->sz = parentproc->sz;

	*(childproc->tf) = *(parentproc->tf);	// copy saved user's registers
	// childproc's ret val of fork need to be set to 0 according to POSIX
	childproc->tf->a0 = 0;

	set_proc_name(childproc, parentproc->name);

	childproc->parent = parentproc;
	childproc->state = READY;
	release(&childproc->lock);

	return childproc->pid;
}

int64_t do_exec()
{
	kprintf("do exec syscall\n");
	return 0;
}