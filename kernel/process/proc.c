#include "proc.h"
#include <kassert.h>
#include <kstring.h>
#include <platform/riscv.h>


struct proc PCBtable[NPROC];
int32_t current = -1;		// current proc
struct proc *idleproc = NULL;	// idle proc
struct proc *initproc = NULL;	// init proc
static int32_t procnum = 0;

void set_proc_name(struct proc *proc, const char *name)
{
	int32_t len = strlen(name);
	assert(len <= PROC_NAME_LEN);
	memcpy(proc->name, name, len);
	proc->name[len] = 0;
}

// initialize the PCB table
void proc_init()
{
	// init the global PCB table
	struct proc *p;
	for (p = PCBtable; p < &PCBtable[NPROC]; p++) {
		p->state = UNUSED;
		p->kstack = 0;
		p->parent = NULL;
	}
	/**
	 * let the booting precedure be a process entity by setting relative
	 * argus
	 */
	idleproc = &PCBtable[0];
	idleproc->pid = 0;
	idleproc->state = RUNNING;
	idleproc->kstack = (uintptr_t)bootstack0;
	write_csr(sscratch, &(PCBtable[0].ctxt));
	set_proc_name(idleproc, "idle");
	current = 0;
	procnum++;
	assert(idleproc != NULL and idleproc->pid == 0);
	// create the 2nd process with pid==1, which is called "init" in UNIX
	initproc = &PCBtable[1];
	initproc->pid = procnum++;
	initproc->state = UNUSED;
	// notice that this is stacktop but not stackbottom!!
	initproc->kstack = (uintptr_t)bootstacktop1;
	initproc->ctxt.sp = initproc->kstack;
	void init_process();
	initproc->ctxt.pc = (uintptr_t)init_process;
	set_proc_name(initproc, "init");
	assert(initproc != NULL and initproc->pid == 1);
}

void scheduler()
{
	extern void switch_to(struct context *);
	// search a process in PCBtable that could be scheduled
	current = (current + 1) % procnum;
	struct context *next = &(PCBtable[current].ctxt);
	switch_to(next);
}

#include <log.h>
void init_process()
{
	extern void delay(int32_t);
	int32_t i = 0;
	while (1) {
		delay(2);
		kprintf("\x1b[%dminit=>%d\x1b[0m ", RED, i++);
	}
}