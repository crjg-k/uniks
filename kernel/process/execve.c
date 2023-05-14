#include "proc.h"
#include <loader/elfloader.h>
#include <mm/memlay.h>
#include <platform/riscv.h>
#include <uniks/defs.h>
#include <uniks/kstring.h>
#include <uniks/param.h>


extern int32_t mappages(pagetable_t pagetable, uintptr_t va, size_t size,
			uintptr_t pa, int32_t perm, int8_t recordref),
	copyin_string(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t max);
extern void *phymem_alloc_page();
extern uintptr_t vaddr2paddr(pagetable_t pagetable, uintptr_t va);


uintptr_t user_stack_argv_envp_bottom =
	USER_STACK_TOP - PGSIZE * USER_STACK_ARGV_ENVP_UPPERLIMIT;

// hint: now, argv/envp string copying couldn't cross page
static int32_t copy_argv_envp(char *filename, char *argv[], char *envp[],
			      uintptr_t string_split, struct trapframe_t *tf)
{
	uintptr_t ustack[MAXARG],
		*current_sp = (uintptr_t *)string_split,
		*current_sp_vaddr = (uintptr_t *)user_stack_argv_envp_bottom,
		arg_env_string_bottom = string_split,
		arg_env_string_bottom_vaddr = user_stack_argv_envp_bottom;
	int32_t argc;
	struct proc_t *p = myproc();
	// push environment strings
	for (argc = 0; envp[argc]; argc++) {
		assert(argc < MAXARG);
		size_t envp_length = strlen((char *)vaddr2paddr(
			p->pagetable, (uintptr_t)envp[argc]));
		if (copyin_string(p->pagetable, (char *)arg_env_string_bottom,
				  (uintptr_t)envp[argc], envp_length + 1) < 0)
			return -1;
		ustack[argc] = arg_env_string_bottom_vaddr;
		arg_env_string_bottom += envp_length + 1;   // '\0' terminate
		arg_env_string_bottom_vaddr +=
			envp_length + 1;		    // '\0' terminate
	}
	ustack[argc] = 0;
	for (; argc >= 0; argc--) {
		*--current_sp = ustack[argc];
		current_sp_vaddr--;
	}
	tf->a2 = (uintptr_t)(current_sp_vaddr);

	// push argument strings
	size_t argv_length;
	for (argc = 0; argv[argc]; argc++) {
		assert(argc < MAXARG);
		argv_length = strlen((char *)vaddr2paddr(
			p->pagetable, (uintptr_t)argv[argc]));
		if (copyin_string(p->pagetable, (char *)arg_env_string_bottom,
				  (uintptr_t)argv[argc], argv_length + 1) < 0)
			return -1;
		ustack[argc] = arg_env_string_bottom_vaddr;
		arg_env_string_bottom += argv_length + 1;   // '\0' terminate
		arg_env_string_bottom_vaddr +=
			argv_length + 1;		    // '\0' terminate
	}
	int32_t ret_argc = argc + 1;
	ustack[argc] = 0;
	for (; argc >= 0; argc--) {
		*--current_sp = ustack[argc];
		current_sp_vaddr--;
	}
	// cope with filename as the first argv[]
	strcpy((char *)arg_env_string_bottom, filename);
	*--current_sp = arg_env_string_bottom_vaddr;
	tf->sp = tf->a1 = (uintptr_t)(--current_sp_vaddr);
	return ret_argc;
}

/**
 * @brief map user stack and copy argc, argv and envp to the top of stack
 * @param p
 * @param pathname
 * @param argv
 * @param envp
 * @return uintptr_t
 */
uintptr_t make_user_stack(struct proc_t *p)
{
	char *pgstart1 = phymem_alloc_page(), *pgstart2 = phymem_alloc_page();
	/**
	 * We currently do not have access to the buddy system allocator, so we
	 * cannot continuously allocate n pages. Let's assume that this can be
	 * allocated to consecutive n pages.
	 */
	assert(pgstart2 - pgstart1 == PGSIZE);
	int32_t perm = PTE_W | PTE_R | PTE_U;
	mappages(p->pagetable, user_stack_argv_envp_bottom - PGSIZE, PGSIZE * 2,
		 (uintptr_t)pgstart1, perm, 1);
	return (uintptr_t)pgstart2;
}

int64_t do_execve(char *pathname, char *argv[], char *envp[])
{
	struct proc_t *p = myproc();
	int64_t ret = 0, entry;
	// todo: release the memory of the original program


	acquire(&pcblock[p->pid]);
	ret = copy_argv_envp(pathname, argv, envp, make_user_stack(p), p->tf);
	assert((p->tf->epc = entry = load_elf(NULL)) != INVALID_ELFFORMAT);
	release(&pcblock[p->pid]);

	sfence_vma();	//  need to flush all TLB
	return ret;
}