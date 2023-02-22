#include "proc.h"
#include "param.h"
#include <kassert.h>
#include <kstring.h>


struct proc PCBtable[NPROC];
struct proc *current = NULL;   // current proc
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
	struct proc *p, *idleproc = &PCBtable[0];
	for (p = PCBtable; p < &PCBtable[NPROC]; p++) {
		p->state = UNUSED;
		p->kstack = (uintptr_t)bootstack;
	}
	idleproc->pid = 0;
	idleproc->state = READY;
	idleproc->kstack = (uintptr_t)bootstack;
	set_proc_name(idleproc, "idle");
	current = idleproc;
	procnum++;
	assert(idleproc != NULL and idleproc->pid == 0);
}