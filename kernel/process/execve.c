#include "proc.h"
#include <fs/ext2fs.h>
#include <loader/elfloader.h>
#include <mm/memlay.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <platform/riscv.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/errno.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>
#include <uniks/param.h>


int32_t copyout_argv_envp(struct proc_t *p, char *argv[], char *envp[],
			  uintptr_t *ustack, char *sp, uint16_t *argv_len,
			  uint16_t *envp_len)
{
	int32_t argc, envc;
	// Push argv[].
	for (argc = 0; argv[argc]; argc++) {
		uint32_t len = argv_len[argc];
		if (copyout(p->mm->pagetable, sp, argv[argc], len) < 0)
			goto ret;
		*(ustack++) = (uintptr_t)sp;
		sp += len;
	}
	*(ustack++) = 0;

	// Push envp[].
	for (envc = 0; envp[envc]; envc++) {
		uint32_t len = envp_len[envc];
		if (copyout(p->mm->pagetable, sp, envp[envc], len) < 0)
			goto ret;
		*(ustack++) = (uintptr_t)sp;
		sp += len;
	}
	*ustack = 0;

	return 0;

ret:
	return -1;
}

void map_user_stack(struct proc_t *p, int32_t page_num, uintptr_t ustack_pa)
{
	int32_t perm = PTE_W | PTE_R | PTE_U;
	p->mm->start_ustack = USER_STACK_TOP;
	mappages(p->mm->pagetable, USER_STACK_TOP - PGSIZE * page_num,
		 PGSIZE * page_num, ustack_pa, perm);
	add_vm_area(p->mm, USER_STACK_TOP - PGSIZE * p->mm->stack_maxsize,
		    USER_STACK_TOP, PTE_R | PTE_W | PTE_U, 0, NULL, 0);
}

int64_t do_execve(struct proc_t *p, char *path, char *argv[], char *envp[])
{
	int64_t res = -EINVAL;
	uint64_t entry;

	struct m_inode_t *inode = namei(path, 1);
	if (inode == NULL)
		goto ret3;

	struct mm_struct *new_mm = new_mm_struct();
	if (new_mm == NULL)
		goto ret3;

	char **tmp_envp, **tmp_argv;
	uint16_t *envp_len, *argv_len;
	if ((entry = load_elf(new_mm, inode)) == -1) {
		// load ELF failed then free new vm_area_list
		free_mm_struct(new_mm);
		goto ret3;
	} else {
		/**
		 * @brief load ELF success then copy argv/envp strings and free
		 * original vm_area_list
		 */
		int32_t argc, envc, totalen = 0, len;
		tmp_envp = kzalloc((MAXARG + 1) * sizeof(uintptr_t));
		envp_len = kmalloc(MAXARG * sizeof(uint16_t));
		for (envc = 0; envc < MAXARG; envc++) {
			// copy vaddr of string
			if (verify_area(p->mm, (uintptr_t)(envp + envc),
					sizeof(uintptr_t), PTE_R | PTE_U) < 0) {
				res = -EFAULT;
				goto ret2;
			}
			assert(copyin(p->mm->pagetable, &tmp_envp[envc],
				      envp + envc, sizeof(uintptr_t)) != -1);
			if (tmp_envp[envc] == NULL)
				break;
			char *envp_va = kmalloc(MAXARGLEN);
			// fetch string by vaddr copyed into tmp_envp[envc]
			if ((len = argstrfetch((uintptr_t)tmp_envp[envc],
					       envp_va, MAXARGLEN)) < 0)
				goto ret2;
			tmp_envp[envc] = envp_va;
			envp_len[envc] = len + 1;
			totalen += envp_len[envc];
			totalen += sizeof(uintptr_t);
		}
		tmp_envp[envc++] = NULL;
		totalen += sizeof(uintptr_t);

		tmp_argv = kzalloc((MAXARG + 1) * sizeof(uintptr_t));
		argv_len = kmalloc(MAXARG * sizeof(uint16_t));
		for (argc = 0; argc < MAXARG; argc++) {
			// copy vaddr of string
			if (verify_area(p->mm, (uintptr_t)(argv + argc),
					sizeof(uintptr_t), PTE_R | PTE_U) < 0) {
				res = -EFAULT;
				goto ret1;
			}
			assert(copyin(p->mm->pagetable, &tmp_argv[argc],
				      argv + argc, sizeof(uintptr_t)) != -1);
			if (tmp_argv[argc] == NULL)
				break;
			char *argv_va = kmalloc(MAXARGLEN);
			// fetch string by vaddr copyed into tmp_argv[argc]
			if ((len = argstrfetch((uintptr_t)tmp_argv[argc],
					       argv_va, MAXARGLEN)) < 0)
				goto ret1;
			tmp_argv[argc] = argv_va;
			argv_len[argc] = len + 1;
			totalen += argv_len[argc];
			totalen += sizeof(uintptr_t);
		}
		tmp_argv[argc++] = NULL;
		totalen += sizeof(uintptr_t);

		totalen = div_round_up(totalen, PGSIZE);   // page number
		uintptr_t *ustack = pages_alloc(totalen);
		if (ustack == NULL) {
			res = -ENOMEM;
			goto ret1;
		}
		p->tf->sp = p->tf->a1 = USER_STACK_TOP - PGSIZE * totalen;
		p->tf->a2 = p->tf->a1 + argc * sizeof(uintptr_t);
		char *sp =
			(char *)(p->tf->a1 + (envc + argc) * sizeof(uintptr_t));

		new_mm->kstack = p->mm->kstack;
		free_pgtable(p->mm->pagetable, 0);
		free_mm_struct(p->mm);
		p->mm = new_mm;
		user_basic_pagetable(p);

		map_user_stack(p, totalen, (uintptr_t)ustack);
		assert(copyout_argv_envp(p, tmp_argv, tmp_envp, ustack, sp,
					 argv_len, envp_len) == 0);
		res = argc - 1;
		p->tf->epc = entry;
		p->name = path;
	}

ret1:
	kfree(argv_len);
	for (int32_t i = 0; tmp_argv[i]; i++)
		kfree(tmp_argv[i]);
ret2:
	kfree(envp_len);
	for (int32_t i = 0; tmp_envp[i]; i++)
		kfree(tmp_envp[i]);
ret3:
	iput(inode);
	return res;
}