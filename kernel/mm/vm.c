#include "vm.h"
#include "memlay.h"
#include "mmu.h"
#include <fs/ext2fs.h>
#include <mm/phys.h>
#include <platform/platform.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>
#include <uniks/list.h>
#include <uniks/param.h>


pagetable_t kernel_pagetable;


void kvmenablehart()
{
	// wait for any previous writes to the page table memory to finish.
	sfence_vma();
	W_SATP(MAKE_SATP(kernel_pagetable, 0));
	// flush stale entries from the TLB.
	sfence_vma();
}

/**
 * @brief Return the address of the PTE in pagetable that corresponds to virtual
 * address va.
 * If alloc!=0, create any required page table pages.
 * @param pagetable
 * @param va
 * @param alloc wether allow to allocate memory
 * @return pgtable_entry_t *: the 3rd level pte
 */
struct pgtable_entry_t *walk(pagetable_t pagetable, uint64_t va, int32_t alloc)
{
	assert(va < MAXVA);

	for (int32_t level = 2; level > 0; level--) {
		struct pgtable_entry_t *pte =
			(struct pgtable_entry_t *)&pagetable[PX(level, va)];
		if (pte->valid) {
			pagetable = (pagetable_t)PNO2PA(pte->paddr);
		} else {
			// this page is not valid
			if (!alloc or (pagetable = pages_zalloc(1)) == NULL)
				return NULL;
			*(pte_t *)pte = PA2PTE(pagetable) | (level ? PTE_V : 0);
		}
	}
	return (struct pgtable_entry_t *)&pagetable[PX(0, va)];
}

/**
 * @brief look up a vaddr in user space, return the paddr of page it is
 * located, or 0 if not mapped
 * @param pagetable
 * @param va
 * @return uintptr_t
 */
static uintptr_t walkaddr(pagetable_t pagetable, uintptr_t va)
{
	struct pgtable_entry_t *pte = walk(pagetable, va, 0);
	if (pte == NULL)
		return 0;
	if (!pte->valid)
		return 0;
	if (!pte->user)
		return 0;
	return PNO2PA(pte->paddr);
}

uintptr_t vaddr2paddr(pagetable_t pagetable, uintptr_t va)
{
	uintptr_t pgstart = walkaddr(pagetable, va);
	if (pgstart == 0)
		return 0;
	return OFFSETPAGE(va) + pgstart;
}

int32_t mappages(pagetable_t pagetable, uintptr_t va, size_t size, uintptr_t pa,
		 int32_t perm)
{
	assert(size != 0);

	uint64_t a = PGROUNDDOWN(va), last = PGROUNDUP(a + size - 1);
	struct pgtable_entry_t *pte;
	while (a < last) {
		if ((pte = walk(pagetable, a, 1)) == NULL)
			return -1;
		/**
		 * @brief assert that this entry does not
		 * refer to a physical memory
		 */
		assert(pte->valid == 0);
		if (va >= TRAMPOLINE)
			set_var_bit(perm, PTE_G);
		*(pte_t *)pte = PA2PTE(pa) | perm | PTE_V;
		if (va >= TRAPFRAME)
			pte->unrelease = 1;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

static pagetable_t kvmmake()
{
	pagetable_t kpgtbl;
	kpgtbl = (pagetable_t)pages_zalloc(1);
	assert(kpgtbl != NULL);
	// uart registers
	assert(mappages(kpgtbl, UART0, PGSIZE, UART0, PTE_R | PTE_W) != -1);
	// virtio mmio disk interface
	assert(mappages(kpgtbl, VIRTIO0, PGSIZE, VIRTIO0, PTE_R | PTE_W) != -1);
	// PLIC
	assert(mappages(kpgtbl, PLIC, 0x400000, PLIC, PTE_R | PTE_W) != -1);

#define KERNELBASE (uintptr_t) KERNEL_BASE_ADDR
	// map kernel text segment
	assert(mappages(kpgtbl, KERNELBASE, (uint64_t)(etext - KERNELBASE),
			KERNELBASE, PTE_R | PTE_X) != -1);
	// map kernel data segment(include stack) and the remainder physical RAM
	assert(mappages(kpgtbl, (uint64_t)etext, PHYSTOP - (uint64_t)etext,
			(uint64_t)etext, PTE_R | PTE_W) != -1);
	// map the trampoline to the highest virtual address in the kernel
	assert(mappages(kpgtbl, TRAMPOLINE, PGSIZE, (uint64_t)trampoline,
			PTE_R | PTE_X) != -1);

	return kpgtbl;
}

// all harts share the same kernel virtual addr space
void kvminit()
{
	kernel_pagetable = kvmmake();
}

/**
 * @brief create an empty (fill with 0) user-process's page table and return
 * NULL if there are no more available pages.
 * @return pagetable_t
 */
pagetable_t uvmcreate()
{
	return (pagetable_t)pages_zalloc(1);
}

void release_phypg(void *phypg)
{
	if (pages_undup(phypg) == 1) {
		release_pglock(phypg);
		pages_free(phypg);
	} else
		release_pglock(phypg);
}

/**
 * @brief Compeletely free user memory pages, then free page-table pages. Except
 * trapframe page.
 * @param pagetable
 * @param level
 */
void free_pgtable(pagetable_t pagetable, int32_t layer)
{
	for (int32_t i = 0; i < PTENUM; i++) {
		struct pgtable_entry_t *pte =
			(struct pgtable_entry_t *)&pagetable[i];
		void *child = (void *)PNO2PA(pte->paddr);
		if (layer < 2) {
			if (pte->valid)	  // this pte has child layer
				free_pgtable(child, layer + 1);
		} else if (layer == 2) {
			if (pte->unrelease)
				continue;
			if (pte->valid)	  // this pte has child layer
				release_phypg(child);
		} else
			BUG();
	}
}

static int64_t copy_pgtable(pagetable_t new_pagetable,
			    pagetable_t old_pagetable, int32_t layer,
			    uint32_t perm)
{
	assert(layer <= 2);

	for (int32_t i = 0; i < PTENUM; i++) {
		struct pgtable_entry_t *old_pte =
			(struct pgtable_entry_t *)&old_pagetable[i];
		if (old_pte->unrelease)
			continue;
		void *old_child = (void *)PNO2PA(old_pte->paddr);
		if (old_pte->valid and layer < 2) {
			// this old_pte has child layer
			void *new_child;
			struct pgtable_entry_t *new_pte =
				(struct pgtable_entry_t *)&new_pagetable[i];
			if (new_pte->valid)
				new_child = (void *)PNO2PA(new_pte->paddr);
			else {
				if ((new_child = pages_zalloc(1)) == NULL)
					return -1;
				new_pagetable[i] =
					PA2PTE(new_child) | PTE_FLAGS(old_pte);
			}
			if (copy_pgtable(new_child, old_child, layer + 1,
					 perm) < 0)
				return -1;
		} else if (old_pte->valid and layer == 2) {
			// this old_pte maps to a physical page
			clear_var_bit(old_pte->perm, perm);
			new_pagetable[i] = old_pagetable[i];
			pages_dup(old_child);
		}
	}

	return 0;
}

/**
 * @brief receive a parent process's mm_struct, and copy its mm_struct only into
 * child's address space without physical memory page it refers to (since COW
 * mechanism is implemented).
 * @param new_mm
 * @param old_mm
 * @return int64_t
 */
int64_t uvm_space_copy(struct mm_struct *new_mm, struct mm_struct *old_mm)
{
	// copy mm_struct
	acquire(&old_mm->mmap_lk);
	new_mm->map_count = old_mm->map_count;
	new_mm->stack_maxsize = old_mm->stack_maxsize;
	new_mm->start_ustack = old_mm->start_ustack;
	new_mm->mmap_base = old_mm->mmap_base;
	new_mm->start_brk = old_mm->start_brk;
	new_mm->brk = old_mm->brk;

	// copy vm_area_struct
	struct vm_area_struct *old_vma, *new_vma;
	struct list_node_t *old_node = list_next(&old_mm->vm_area_list_head),
			   *new_node = &new_mm->vm_area_list_head;
	while (old_node != &old_mm->vm_area_list_head) {
		old_vma = element_entry(old_node, struct vm_area_struct,
					vm_area_list);
		new_vma = new_vmarea_struct(
			old_vma->vm_start, old_vma->vm_end, old_vma->vm_flags,
			old_vma->vm_pgoff, old_vma->vm_inode, old_vma->_filesz);

		new_vma->vm_mm = new_mm;
		list_add_front(&new_vma->vm_area_list, new_node);
		old_node = list_next(old_node);
		new_node = list_next(new_node);

		for (uintptr_t va = old_vma->vm_start; va < old_vma->vm_end;
		     va += PGSIZE)
			invalidate(va);
	}

	copy_pgtable(new_mm->pagetable, old_mm->pagetable, 0, PTE_W);

	release(&old_mm->mmap_lk);

	return 0;
}

struct mm_struct *new_mm_struct()
{
	struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));

	if ((mm->pagetable = uvmcreate()) == NULL)
		return NULL;
	initlock(&mm->mmap_lk, "mmlock");
	INIT_LIST_HEAD(&mm->vm_area_list_head);
	mm->map_count = 0;
	mm->mm_count = 1;
	mm->stack_maxsize = MAXSTACK;

	return mm;
}

/**
 * @brief free an mm_struct with its vm_area_list
 * @param mm
 */
void free_mm_struct(struct mm_struct *mm)
{
	// free all vm_area structs linked in mm
	struct vm_area_struct *vma;
	acquire(&mm->mmap_lk);
	struct list_node_t *vm_area_node = list_next(&mm->vm_area_list_head);
	while (vm_area_node != &mm->vm_area_list_head) {
		vma = element_entry(vm_area_node, struct vm_area_struct,
				    vm_area_list);
		iput(vma->vm_inode);
		kfree(vma);
		vm_area_node = list_next(vm_area_node);
	}
	release(&mm->mmap_lk);

	kfree(mm);
}

struct vm_area_struct *new_vmarea_struct(uintptr_t _va_start, uintptr_t _va_end,
					 uint64_t flags, uint64_t pgoff,
					 struct m_inode_t *inode,
					 uint64_t _filesz)
{
	struct vm_area_struct *vma = kmalloc(sizeof(struct vm_area_struct));

	INIT_LIST_HEAD(&vma->vm_area_list);
	vma->vm_start = _va_start;
	vma->vm_end = _va_end;
	vma->vm_flags = flags;
	vma->vm_inode = idup(inode);
	vma->vm_pgoff = pgoff;
	vma->_filesz = _filesz;

	return vma;
}

int32_t add_vm_area(struct mm_struct *mm, uintptr_t _va_start,
		    uintptr_t _va_end, uint64_t flags, uint64_t pgoff,
		    struct m_inode_t *inode, uint64_t _filesz)
{
	assert(OFFSETPAGE(_va_start) == 0);   // align to page
	assert(OFFSETPAGE(_va_end) == 0);     // align to page

	struct vm_area_struct *vma;

	acquire(&mm->mmap_lk);
	// "insert sorting"
	struct list_node_t *vm_area_node = list_next(&mm->vm_area_list_head);
	while (vm_area_node != &mm->vm_area_list_head) {
		vma = element_entry(vm_area_node, struct vm_area_struct,
				    vm_area_list);
		assert(_va_end <= vma->vm_start or _va_start >= vma->vm_end);
		if (vma->vm_start >= _va_end)
			break;
		vm_area_node = list_next(vm_area_node);
	}

	vma = new_vmarea_struct(_va_start, _va_end, flags, pgoff, inode,
				_filesz);
	vma->vm_mm = mm;
	mm->map_count++;
	list_add_tail(&vma->vm_area_list, vm_area_node);

	release(&mm->mmap_lk);
	return 0;
}

struct vm_area_struct *search_vmareas(struct mm_struct *mm,
				      uintptr_t target_vaddr, size_t size,
				      uint32_t *flag_res)
{
	uint32_t perm = 0;
	uintptr_t end_vaddr = target_vaddr + size;

	struct vm_area_struct *vma;
	acquire(&mm->mmap_lk);
	struct list_node_t *vm_area_node = list_next(&mm->vm_area_list_head);
	while (vm_area_node != &mm->vm_area_list_head) {
		vma = element_entry(vm_area_node, struct vm_area_struct,
				    vm_area_list);
		if (target_vaddr >= vma->vm_start and
		    end_vaddr <= vma->vm_end) {
			set_var_bit(perm, vma->vm_flags);
			break;
		}
		vm_area_node = list_next(vm_area_node);
	}
	if (vm_area_node == &mm->vm_area_list_head)
		vma = NULL;
	release(&mm->mmap_lk);

	*flag_res = perm;
	return vma;
}

static void true_load_segment(struct vm_area_struct *vma, uintptr_t vaddr,
			      char *page_start)
{
	assert(OFFSETPAGE(vaddr) == 0);
	uint32_t segoff = vaddr - vma->vm_start;
	uint32_t pgoff = MAX(segoff, vma->_filesz) - segoff;
	if (pgoff > PGSIZE)
		pgoff = PGSIZE;

	if (segoff < vma->_filesz) {
		ilock(vma->vm_inode);
		readi(vma->vm_inode, 0, page_start, vma->vm_pgoff + segoff,
		      pgoff);
		iunlock(vma->vm_inode);
	}
	if (vma->_filesz != (vma->vm_end - vma->vm_start)) {
		// means that .bss section
		memset(page_start + pgoff, 0, PGSIZE - pgoff);
	}
}

// this function's name comes from Linux v1.0
int64_t do_no_page(struct mm_struct *mm, struct vm_area_struct *vma,
		   uintptr_t vaddr, uint32_t targetperm)
{
	char *page_start = pages_alloc(1);
	if (page_start == NULL)
		return -1;
	mappages(mm->pagetable, vaddr, PGSIZE, (uintptr_t)page_start,
		 targetperm);
	if (vma->vm_inode != NULL)
		true_load_segment(vma, vaddr, page_start);
	return 0;
}

// this function's name comes from Linux v1.0
int64_t do_wp_page(struct pgtable_entry_t *pte, uintptr_t vaddr,
		   uint32_t targetperm)
{
	char *physpg_paddr = (char *)PNO2PA(pte->paddr);

	/**
	 * @brief Only being referenced once indicates that it has not been
	 * shared, so just change the read and write properties.
	 */
	if (pages_undup(physpg_paddr) == 1) {
		set_var_bit(pte->perm, targetperm);
		release_pglock(physpg_paddr);
		goto ret;
	}

	/**
	 * @brief If it is referenced multiple times, it is necessary to
	 * copy the physical page table.
	 */
	char *new_page = pages_alloc(1);
	if (new_page == NULL) {
		release_pglock(physpg_paddr);
		return -1;
	}
	memcpy(new_page, physpg_paddr, PGSIZE);
	release_pglock(physpg_paddr);
	set_var_bit(targetperm, pte->perm);
	*(pte_t *)pte = PA2PTE(new_page) | targetperm;

ret:
	invalidate(vaddr);   // Don't forget to flush TLB
	return 0;
}

/**
 * @brief verify process p's vm-space before writing to user space, and
 * the name of this function comes from Linux-0.11
 * @param p
 * @param vaddr
 * @param size
 * @return int32_t: -1 if segment fault; -2 if out of memory
 */
int32_t verify_area(struct mm_struct *mm, uintptr_t vaddr, size_t size,
		    int32_t targetperm)
{
	uint32_t vm_flags, res;

	struct vm_area_struct *vma = search_vmareas(mm, vaddr, size, &vm_flags);

	/**
	 * @brief If the [vaddr, vaddr+size) area's permission is not the
	 * superset of targetperm, then trigger segment fault.
	 */
	if (vma == NULL or get_var_bit(targetperm, vm_flags) != targetperm) {
		return -1;
	} else {
		uintptr_t end_vaddr = vaddr + size;
		for (uintptr_t start_vaddr = PGROUNDDOWN(vaddr);
		     start_vaddr < end_vaddr; start_vaddr += PGSIZE) {
			res = 0;
			struct pgtable_entry_t *pte =
				walk(mm->pagetable, start_vaddr, 1);
			if (pte == NULL)
				return -2;
			if (get_var_bit(pte->perm, targetperm) == targetperm)
				continue;

			// handling of COW mechanism
			if (pte->valid)
				res = do_wp_page(pte, start_vaddr, targetperm);
			else {
				/**
				 * @brief Pages that have not been loaded into
				 * memory must not have been referenced multiple
				 * times by the COW mechanism.
				 */
				res = do_no_page(mm, vma, start_vaddr,
						 targetperm);
			}
			if (res < 0)
				return -2;
		}
	}

	return 0;
}

#define SEGFAULT_MSG "Segmentation fault"
#define SEG_FAULT(pid, inst, vaddr) \
	({ \
		kprintf("\n\x1b[%dmpid[%d] %s:%p=>%p\x1b[0m\n", RED, pid, \
			SEGFAULT_MSG, inst, vaddr); \
	})
#define OOM_MSG	    "Out of memory"
#define OOM_FAULT() ({ kprintf("\n\x1b[%dm%s\x1b[0m\n", RED, OOM_MSG); })
void do_inst_page_fault(uintptr_t fault_vaddr)
{
	int64_t res;
	struct proc_t *p = myproc();
	if ((res = verify_area(p->mm, fault_vaddr, 4, PTE_X | PTE_R | PTE_U)) ==
	    -1) {
		SEG_FAULT(p->pid, p->tf->epc, fault_vaddr);
		setkilled(p);
	} else if (res == -2) {
		OOM_FAULT();
		setkilled(p);
	}
}

/**
 * @brief On account of RISCV requiring addresses' alignment of LOAD and STROE
 * instructions, this function could just regards accessing size as 1.
 * @param fault_vaddr
 */
void do_ld_page_fault(uintptr_t fault_vaddr)
{
	int64_t res;
	struct proc_t *p = myproc();
	if ((res = verify_area(p->mm, fault_vaddr, 1, PTE_R | PTE_U)) == -1) {
		SEG_FAULT(p->pid, p->tf->epc, fault_vaddr);
		setkilled(p);
	} else if (res == -2) {
		OOM_FAULT();
		setkilled(p);
	}
}

/**
 * @brief On account of RISCV requiring addresses' alignment of LOAD and STROE
 * instructions, this function could just regards accessing size as 1.
 * @param fault_vaddr
 */
void do_sd_page_fault(uintptr_t fault_vaddr)
{
	int64_t res;
	struct proc_t *p = myproc();
	if ((res = verify_area(p->mm, fault_vaddr, 1, PTE_R | PTE_W | PTE_U)) ==
	    -1) {
		SEG_FAULT(p->pid, p->tf->epc, fault_vaddr);
		setkilled(p);
	} else if (res == -2) {
		OOM_FAULT();
		setkilled(p);
	}
}

// `int brk(void *addr);`
int64_t sys_brk()
{
	struct proc_t *p = myproc();
	struct mm_struct *mm = p->mm;
	uintptr_t vaddr = argufetch(p, 0);
	if (vaddr <= mm->start_brk)
		goto ret;

	// .text, .data(include .bss), .heap, .stack
	assert(mm->map_count >= 4);

	// search the heap vm_area_struct
	struct vm_area_struct *vma;
	int32_t idx = 0;
	acquire(&mm->mmap_lk);
	struct list_node_t *vm_area_node = list_next(&mm->vm_area_list_head);
	while (1) {
		if (idx == 2) {
			vma = element_entry(vm_area_node, struct vm_area_struct,
					    vm_area_list);
			break;
		}
		vm_area_node = list_next(vm_area_node);
		idx++;
	}
	mm->brk = vma->vm_end = vaddr;
	release(&mm->mmap_lk);

ret:
	return mm->brk;
}


/* === transmit data between user space and kernel space === */

/**
 * @brief Copy from kernel to user. Copy len bytes to dst from virtual
 * address srcva in a given page table.
 * @param pagetable
 * @param dst
 * @param srcva
 * @param len
 * @return int32_t: 0 on success, -1 on error.
 */
int32_t copyin(pagetable_t pagetable, void *dst, void *srcva, uint64_t len)
{
	uint64_t n, va0, pa0;
	uintptr_t src_vaddr = (uintptr_t)srcva;

	while (len > 0) {
		va0 = PGROUNDDOWN(src_vaddr);
		if ((pa0 = walkaddr(pagetable, va0)) == 0)
			return -1;
		n = PGSIZE - (src_vaddr - va0);
		if (n > len)
			n = len;
		memcpy(dst, (void *)(pa0 + (src_vaddr - va0)), n);

		len -= n;
		dst += n;
		src_vaddr = va0 + PGSIZE;
	}
	return 0;
}

/**
 * @brief Copy a null-terminated string from user to kernel.
 * Copy bytes to dst from virtual address srcva in a given page table,
 * until a '\0', or max.
 * @param pagetable
 * @param dst
 * @param srcva
 * @param max
 * @return int32_t: `strlen(dst)` on success, -1 on error.
 */
int32_t copyin_string(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t max)
{
	uint64_t n, va0, pa0, got_null = 0;
	char *dstart = dst;

	while (got_null == 0 and max > 0) {
		va0 = PGROUNDDOWN(srcva);
		if ((pa0 = walkaddr(pagetable, va0)) == 0)
			return -1;
		n = PGSIZE - (srcva - va0);
		if (n > max)
			n = max;

		char *p = (char *)(pa0 + (srcva - va0));
		while (n > 0) {
			if (*p == '\0') {
				*dst = '\0';
				got_null = 1;
				break;
			} else {
				*dst = *p;
			}
			--n;
			--max;
			p++;
			dst++;
		}
		srcva = va0 + PGSIZE;
	}
	return got_null ? (dst - dstart) : -1;
}

/**
 * @brief Copy from kernel to user. Copy len bytes from src to virtual
 * address dstva in a given page table.
 * @param pagetable
 * @param dstva
 * @param src
 * @param len
 * @return int32_t: 0 on success, -1 on error.
 */
int32_t copyout(pagetable_t pagetable, void *dstva, void *src, uint64_t len)
{
	uint64_t n, va0, pa0;
	uintptr_t dst_vaddr = (uintptr_t)dstva;

	while (len > 0) {
		va0 = PGROUNDDOWN(dst_vaddr);
		if ((pa0 = walkaddr(pagetable, va0)) == 0)
			return -1;
		n = PGSIZE - (dst_vaddr - va0);
		if (n > len)
			n = len;
		memcpy((void *)(pa0 + (dst_vaddr - va0)), src, n);

		len -= n;
		src += n;
		dst_vaddr = va0 + PGSIZE;
	}
	return 0;
}

/**
 * @brief Copy from either a user address, or kernel address, depending
 * on usr_src.
 * @param user_src
 * @param dst
 * @param src
 * @param len
 * @return int32_t: 0 on success, -1 on error.
 */
int32_t either_copyin(int32_t user_src, void *dst, void *src, uint64_t len)
{
	struct proc_t *p = myproc();
	if (user_src)
		return copyin(p->mm->pagetable, dst, src, len);
	else {
		memcpy(dst, src, len);
		return 0;
	}
}

/**
 * @brief Copy to either a user address, or kernel address, depending on
 * usr_dst.
 * @param user_dst
 * @param dst
 * @param src
 * @param len
 * @return int32_t: 0 on success, -1 on error.
 */
int32_t either_copyout(int32_t user_dst, void *dst, void *src, uint64_t len)
{
	struct proc_t *p = myproc();
	if (user_dst)
		return copyout(p->mm->pagetable, dst, src, len);
	else {
		memcpy(dst, src, len);
		return 0;
	}
}