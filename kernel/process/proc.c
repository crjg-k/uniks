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

// create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t proc_pagetable(struct proc *p)
{
	pagetable_t pagetable = uvmcreate();
	if (pagetable == 0)
		return 0;

	// map the trampoline code (for system call return)
	// at the highest user virtual address.
	// only the supervisor uses it, on the way
	// to/from user space, so not PTE_U.
	if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64_t)trampoline,
		     PTE_R | PTE_X) < 0) {
		uvmfree(pagetable, 0);
		return 0;
	}

	// map the trapframe page just below the trampoline page, for
	// trampoline.S.
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

// Allocate a page for each process's kernel stack.
// Map it's high in memory, followed by an invalid guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
	for (struct proc *p = pcbtable; p < &pcbtable[NPROC]; p++) {
		char *pa = kalloc();
		assert(pa != 0);
		uint64_t va = KSTACK((int32_t)(p - pcbtable));
		mappages(kpgtbl, va, PGSIZE, (uint64_t)pa, PTE_R | PTE_W);
	}
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void freeproc(struct proc *p)
{
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
 * @brief allocate a new process and fill the switch tiny context between kernel
 * process
 *
 * @return struct proc*: new allocate process's pcb entry in pcb table
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
	if ((p->pagetable = proc_pagetable(p)) == 0) {
		// if there are no enough memory, failed
		freeproc(p);
		release(&p->lock);
		return NULL;
	}

	// set up new context to start executing at forkret,
	// which returns to user space
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

/**
 * @brief each hart will hold a local scheduler context
 */
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
				// before jumping back to us.
				c->proc = p;
				p->state = RUNNING;
				switch_to(&c->ctxt, &p->ctxt);

				// process is done running for now since timer
				// interrupt.
				c->proc = NULL;
			}
			release(&p->lock);
		}
	}
}

// switch to kernel thread scheduler in per hart
// must hold p->lock
void sched(void)
{
	struct proc *p = myproc();

	assert(holding(&p->lock));
	assert(interrupt_get() == 0);

	p->state = READY;
	switch_to(&p->ctxt, &mycpu()->ctxt);
}

// give up the CPU
void yield()
{
	struct proc *p = myproc();
	acquire(&p->lock);

	sched();
	release(&p->lock);
}

// a user program that calls fork() then write() which is assembled from
// ${cwd}/user/initcode.S
uint32_t initcode[] = {0x00100893, 0x00000073, 0x00051863, 0x03000593,
		       0x03200613, 0x00c584a3, 0x03000593, 0x00c00613,
		       0x01000893, 0x00000073, 0xff1ff06f, 0x00000000,
		       0x74696e69, 0x636f7270, 0x000a3120, 0x00000000};

void user_init()
{
	struct proc *p = allocproc();
	initproc = p;

	// allocate one user page and copy initcode's instructions
	// and data into it
	uvmfirst(p->pagetable, initcode, sizeof(initcode));
	p->sz = PGSIZE;

	// forgery context: prepare for the 1st time return from kernel to user
	p->tf->epc = 0;
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
	// copy user memory from parent to child.
	if (uvmcopy(parentproc->pagetable, childproc->pagetable,
		    parentproc->sz) < 0) {
		freeproc(childproc);
		release(&childproc->lock);
		return -1;
	}
	childproc->sz = parentproc->sz;

	*(childproc->tf) = *(parentproc->tf);	// copy saved user registers

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