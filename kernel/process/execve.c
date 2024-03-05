#include "proc.h"
#include <file/file.h>
#include <loader/elfloader.h>
#include <mm/memlay.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <platform/riscv.h>
#include <uniks/defs.h>
#include <uniks/kstring.h>
#include <uniks/param.h>


uintptr_t user_stack_argv_envp_bottom =
	USER_STACK_TOP - PGSIZE * USER_STACK_ARGVENVP_UPPERLIMIT_PGAE;


int32_t copy_argv_envp(struct proc_t *p, char *filename, char *argv[],
		       char *envp[], uintptr_t string_split)
{
	uintptr_t *ustack = kmalloc(MAXARG * sizeof(uintptr_t));
	uintptr_t *current_sp = (uintptr_t *)string_split,
		  *current_sp_vaddr = (uintptr_t *)user_stack_argv_envp_bottom,
		  arg_env_string_bottom = string_split,
		  arg_env_string_bottom_vaddr = user_stack_argv_envp_bottom;
	int32_t argc;

	// push environment strings
	if (envp) {
		for (argc = 0; envp[argc]; argc++) {
			assert(argc < MAXARG);
			char *env = (char *)vaddr2paddr(p->mm->pagetable,
							(uintptr_t)envp[argc]);
			size_t envp_length = strlen(env);
			if (copyin_string(p->mm->pagetable,
					  (char *)arg_env_string_bottom,
					  (uintptr_t)envp[argc],
					  envp_length + 1) < 0)
				return -1;
			ustack[argc] = arg_env_string_bottom_vaddr;
			arg_env_string_bottom +=
				envp_length + 1;   // '\0' terminate
			arg_env_string_bottom_vaddr +=
				envp_length + 1;   // '\0' terminate
		}
	}
	ustack[argc] = 0;
	for (; argc >= 0; argc--) {
		*--current_sp = ustack[argc];
		current_sp_vaddr--;
	}
	p->tf->a2 = (uintptr_t)(current_sp_vaddr);

	// push argument strings
	size_t argv_length;
	if (argv) {
		for (argc = 0; argv[argc]; argc++) {
			assert(argc < MAXARG);
			char *arg = (char *)vaddr2paddr(p->mm->pagetable,
							(uintptr_t)argv[argc]);
			argv_length = strlen(arg);
			if (copyin_string(p->mm->pagetable,
					  (char *)arg_env_string_bottom,
					  (uintptr_t)argv[argc],
					  argv_length + 1) < 0)
				return -1;
			ustack[argc] = arg_env_string_bottom_vaddr;
			arg_env_string_bottom +=
				argv_length + 1;   // '\0' terminate
			arg_env_string_bottom_vaddr +=
				argv_length + 1;   // '\0' terminate
		}
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
	p->tf->sp = p->tf->a1 = (uintptr_t)(--current_sp_vaddr);
	return ret_argc;
}

uintptr_t make_user_stack(struct proc_t *p)
{
	char *pgstart = pages_alloc(2);
	// int32_t perm = PTE_W | PTE_R | PTE_U;
	// mappages(p->mm->pagetable, user_stack_argv_envp_bottom - PGSIZE * 2,
	// 	 PGSIZE * 2, (uintptr_t)pgstart, perm);
	p->tf->sp = USER_STACK_TOP;
	add_vm_area(p->mm, USER_STACK_TOP - PGSIZE * 2, USER_STACK_TOP,
		    PTE_R | PTE_W | PTE_U, 0, NULL);
	return (uintptr_t)pgstart;
}

int64_t do_execve(struct proc_t *p, char *pathname, char *argv[], char *envp[])
{
	int64_t ret = 0;
	struct mm_struct *new_mm = kmalloc(sizeof(struct mm_struct)), *old_mm;
	old_mm = p->mm, p->mm = new_mm;
	init_mm(new_mm);

	acquire(&pcblock[p->pid]);
	// ret = copy_argv_envp(p, pathname, argv, envp,
	// 		     make_user_stack(p) + PGSIZE);
	make_user_stack(p);
	if (ret < 0)
		goto out;
	if (load_elf(p, (struct m_inode_t *)pathname) != 0) {
		// load ELF failed then free new vm_area_list
		free_mm(new_mm);
		p->mm = old_mm;
	} else {
		// load ELF success then free original vm_area_list
		p->mm->kstack = old_mm->kstack;
		user_basic_pagetable(p);
		uvmfree(p->mm);
		free_mm(old_mm);
	}

out:
	release(&pcblock[p->pid]);
	return ret;
}