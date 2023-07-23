#include "vm.h"
#include "memlay.h"
#include "mmu.h"
#include <mm/phys.h>
#include <platform/platform.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>
#include <uniks/list.h>
#include <uniks/param.h>


pagetable_t kernel_pagetable;


void kvmenable()
{
	// wait for any previous writes to the page table memory to finish.
	sfence_vma();
	w_satp(MAKE_SATP(kernel_pagetable));
	// flush stale entries from the TLB.
	sfence_vma();
}

/**
 * @brief return the address of the PTE in pagetable that corresponds to virtual
 * address va.
 * if alloc!=0, create any required page table pages
 *
 * @param pagetable
 * @param va
 * @param alloc wether allow to allocate memory
 * @return pte_t *: the 3rd level pte
 */
static pte_t *walk(pagetable_t pagetable, uint64_t va, int32_t alloc)
{
	assert(va < MAXVA);

	for (int32_t level = 2; level > 0; level--) {
		struct pagetable_entry_t *pte =
			(struct pagetable_entry_t *)&pagetable[PX(level, va)];
		if (pte->valid) {
			pagetable = (pagetable_t)PTE2PA(pte);
		} else {   // this page is not valid
			if (!alloc or
			    (pagetable = (pde_t *)pages_alloc(1, 0)) == NULL)
				return NULL;
			memset(pagetable, 0, PGSIZE);
			*(pte_t *)pte = PA2PTE(pagetable) | (level ? PTE_V : 0);
		}
	}
	return &pagetable[PX(0, va)];
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
	struct pagetable_entry_t *pte =
		(struct pagetable_entry_t *)walk(pagetable, va, 0);
	if (pte == NULL)
		return 0;
	if (!pte->valid)
		return 0;
	if (!pte->user)
		return 0;
	return PTE2PA(pte);
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
	struct pagetable_entry_t *pte;
	while (a < last) {
		if ((pte = (struct pagetable_entry_t *)walk(pagetable, a, 1)) ==
		    NULL)
			return -1;
		/**
		 * @brief assert that this entry does not
		 * refer to a physical memory
		 */
		assert(pte->valid == 0);
		*(pte_t *)pte = PA2PTE(pa) | perm | PTE_V;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

static pagetable_t kvmmake()
{
	pagetable_t kpgtbl;
	kpgtbl = (pagetable_t)pages_alloc(1, 0);
	memset(kpgtbl, 0, PGSIZE);
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

// only initialize the kernel pagetable when booting
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
	pagetable_t pagetable;
	pagetable = pages_alloc(1, 0);
	if (pagetable == NULL)
		return NULL;
	memset(pagetable, 0, PGSIZE);
	return pagetable;
}

/**
 * @brief Set several pages' flags covers [va_start, va_end) in pagetable.
 *
 * @param pagetable
 * @param va_start must be page-aligned namely 4096 bytes aligned
 * @param va_end
 * @param perm
 */
void uvmmap(pagetable_t pagetable, uintptr_t va_start, uintptr_t va_end,
	    int32_t perm)
{
	assert((va_start % PGSIZE) == 0);

	struct pagetable_entry_t *pte;
	for (uintptr_t va = va_start; va < va_end; va += PGSIZE) {
		assert((pte = (struct pagetable_entry_t *)walk(pagetable, va,
							       0)) != NULL);
		pte->perm |= perm;   // set corresponding bits
	}
}

/**
 * @brief Clear several pages' flags covers [va_start, va_end) in pagetable.
 *
 * @param pagetable
 * @param va_start must be page-aligned namely 4096 bytes aligned
 * @param va_end
 * @param perm
 */
void uvmunmap(pagetable_t pagetable, uintptr_t va_start, uintptr_t va_end,
	      int32_t perm)
{
	assert((va_start % PGSIZE) == 0);

	struct pagetable_entry_t *pte;
	for (uintptr_t va = va_start; va < va_end; va += PGSIZE) {
		assert((pte = (struct pagetable_entry_t *)walk(pagetable, va,
							       0)) != NULL);
		pte->perm &= ~perm;   // clear corresponding bits
	}
}

int32_t init_mm(struct mm_struct *mm)
{
	if ((mm->pagetable = uvmcreate()) == NULL)
		return -1;
	initlock(&mm->mmap_lk, "mm_lock");
	INIT_LIST_HEAD(&mm->vm_area_list_head);
	mm->mm_count = mm->map_count = 0;
	struct vm_area_struct *vm0 = kmalloc(sizeof(struct vm_area_struct));
	vm0->vm_start = vm0->vm_end = 0;
	vm0->vm_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U;
	list_add_front(&vm0->vm_area_list, &mm->vm_area_list_head);

	vm0 = kmalloc(sizeof(struct vm_area_struct));
	vm0->vm_start = vm0->vm_end = USER_STACK_TOP;
	vm0->vm_flags = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U;
	list_add_tail(&vm0->vm_area_list, &mm->vm_area_list_head);

	return 0;
}

/**
 * @brief free an mm_struct with its vm_area_list
 *
 * @param mm
 */
void free_mm(struct mm_struct *mm)
{
	kfree(mm);
}

// hint: this will not deal with a bad overlay range!
int32_t add_vm_area(struct mm_struct *mm, uintptr_t va_start, uintptr_t va_end,
		    uint64_t flags, uint64_t pgoff, struct file_t *file)
{
	acquire(&mm->mmap_lk);
	struct vm_area_struct *vm_area_new =
				      kmalloc(sizeof(struct vm_area_struct)),
			      *vm_area_target, *vm_area_target_next;
	INIT_LIST_HEAD(&vm_area_new->vm_area_list);
	vm_area_new->vm_start = va_start;
	vm_area_new->vm_end = va_end;
	vm_area_new->vm_flags = flags;
	vm_area_new->vm_file = file;
	vm_area_new->vm_pgoff = pgoff;
	mm->map_count++;

	// "insert sorting"
	struct list_node_t *vm_area_node = mm->vm_area_list_head.next,
			   *vm_area_node_next = vm_area_node->next;
	while (vm_area_node_next->next != &mm->vm_area_list_head) {
		vm_area_target = element_entry(
			vm_area_node, struct vm_area_struct, vm_area_list);
		vm_area_target_next = element_entry(
			vm_area_node_next, struct vm_area_struct, vm_area_list);
		if (va_start >= vm_area_target->vm_end and
		    va_end < vm_area_target_next->vm_start) {
			break;
		}
		vm_area_node = vm_area_node_next;
		vm_area_node_next = vm_area_node->next;
	}
	list_add_front(&vm_area_new->vm_area_list, vm_area_node);

	release(&mm->mmap_lk);
	return 0;
}

struct vm_area_struct *search_vmareas(struct mm_struct *mm,
				      uintptr_t target_vaddr, size_t size,
				      uint32_t *flag_res)
{
	uint32_t perm = UINT32_MAX;
	uintptr_t end_vaddr = target_vaddr + size;
	acquire(&mm->mmap_lk);
	struct list_node_t *vm_area_node = list_next(&mm->vm_area_list_head);
	// vm_area_first record the first suitable vm_area_struct's addr
	struct vm_area_struct *vm_area, *vm_area_first = NULL;
	for (; vm_area_node != &mm->vm_area_list_head;
	     vm_area_node = list_next(vm_area_node)) {
		vm_area = element_entry(vm_area_node, struct vm_area_struct,
					vm_area_list);
		if (target_vaddr >= vm_area->vm_end)
			continue;
		if (target_vaddr >= vm_area->vm_start and
		    target_vaddr < vm_area->vm_end) {
			if (vm_area_first == NULL)
				vm_area_first = vm_area;
			target_vaddr = vm_area->vm_end;
			perm &= vm_area->vm_flags;
		} else
			break;
	}
	release(&mm->mmap_lk);

	if (target_vaddr < end_vaddr)
		return NULL;
	*flag_res = perm;
	return vm_area_first;
}

void free_pagetable() {}

void free_mm_struct() {}

/**
 * @brief free page table(both pde and pte), and all leaf must already
 * have been removed
 * @param pagetable
 * @param level
 */
void freewalk(pagetable_t pagetable, int32_t level)
{
	if (level < 2) {
		for (int32_t i = 0; i < PTENUM; i++) {
			struct pagetable_entry_t *pte =
				(struct pagetable_entry_t *)&pagetable[i];
			if (pte->valid and
			    !(pte->read and pte->write and pte->execute)) {
				// this pte has child level
				uint64_t child = PTE2PA(pte);
				freewalk((pagetable_t)child, level + 1);
			}
		}
	}
	pages_free(pagetable);
}

// compeletely free user memory pages, then free page-table pages
void uvmfree(struct mm_struct *mm) {}

void copy_pagetable() {}

void copy_mm_struct() {}

/**
 * @brief receive a parent process's page table, and copy its page table
 * only into child's address space without physical memory page it
 * refers to (since the COW mechanism is implemented)
 *
 * @param old namely parent process
 * @param new namely child process
 * @param sz
 * @return int64_t
 */
int64_t uvmcopy(struct mm_struct *mm_new, struct mm_struct *mm_old)
{
	struct pagetable_entry_t *pte;
	uintptr_t va, pa;
	uint32_t flags;

	while (!list_empty(&mm_new->vm_area_list_head)) {
		struct list_node_t *next_node =
			list_next(&mm_new->vm_area_list_head);
		struct vm_area_struct *vma = element_entry(
			next_node, struct vm_area_struct, vm_area_list);
		for (va = vma->vm_start; va < vma->vm_end; va += PGSIZE) {
			assert((pte = (struct pagetable_entry_t *)walk(
					mm_old->pagetable, va, 0)) != NULL);
			assert(pte->valid != 0);
			pa = PTE2PA(pte);
			pte->write = 0;
			flags = PTE_FLAGS(pte);
			// add mapping to childproc pagetable
			assert(mappages(mm_new->pagetable, va, PGSIZE,
					(uintptr_t)pa, flags) != -1);
			// mem_map[PA2ARRAYINDEX(pa)]++;

			invalidate(va);
		}
	}
	return 0;
}

/**
 * @brief verify process p's vm-space before writing to user space, and
 * the name of this function comes from Linux-0.11
 *
 * @param p
 * @param vaddr
 * @param size
 * @return int32_t: -1 if segment fault
 */
int32_t verify_area(struct mm_struct *mm, uintptr_t vaddr, size_t size,
		    int32_t targetperm)
{
	uint32_t vm_flags;

	struct vm_area_struct *vm_area_start =
		search_vmareas(mm, vaddr, size, &vm_flags);

	/**
	 * @brief If the [vaddr, vaddr+size) area's permission is not the
	 * superset of targetperm, then trigger segment fault.
	 */
	if (vm_area_start == NULL or (targetperm & vm_flags) != targetperm) {
		return -1;
	} else {
		pte_t *pte = walk(mm->pagetable, vaddr, 1);
		if (!get_variable_bit(*pte, PTE_V)) {
			size_t npages = div_round_up(size, PGSIZE);
			char *page_start = pages_alloc(npages, 0);
			mappages(mm->pagetable, vaddr, PGSIZE * npages,
				 (uintptr_t)page_start, targetperm | PTE_V);
			if (vm_area_start->vm_file != NULL) {
				memcpy(page_start,
				       (void *)vm_area_start->vm_file +
					       vm_area_start->vm_pgoff,
				       PGSIZE * npages);
			}
		} else {
			uvmmap(mm->pagetable, PGROUNDDOWN(vaddr),
			       PGROUNDDOWN(vaddr) + 1, targetperm);
		}
	}

	sfence_vma();	//  need to flush all TLB
	return 0;
}

#define SEGFAULT_MSG	     "Segmentation fault"
#define segmentation_fault() ({ kprintf("%s\n", SEGFAULT_MSG); })
void do_inst_page_fault(struct mm_struct *mm, uintptr_t fault_vaddr)
{
	if (verify_area(mm, fault_vaddr, 4, PTE_X | PTE_R | PTE_U) != 0)
		segmentation_fault();
}
// todo: distinguish byte/half word/word/double word
void do_ld_page_fault(struct mm_struct *mm, uintptr_t fault_vaddr)
{
	if (verify_area(mm, fault_vaddr, 8, PTE_R | PTE_W | PTE_U) != 0)
		segmentation_fault();
}
void do_sd_page_fault(struct mm_struct *mm, uintptr_t fault_vaddr)
{
	if (verify_area(mm, fault_vaddr, 8, PTE_R | PTE_W | PTE_U) != 0)
		segmentation_fault();
}

/* === transmit data between user space and kernel space === */

/**
 * @brief Copy from kernel to user. Copy len bytes to dst from virtual
 * address srcva in a given page table. And return 0 on success, -1 on
 * error
 * @param pagetable
 * @param dst
 * @param srcva
 * @param len
 * @return int64_t
 */
int64_t copyin(pagetable_t pagetable, void *dst, void *srcva, uint64_t len)
{
	uint64_t n, va0, pa0;
	uintptr_t src_vaddr = (uintptr_t)srcva;

	while (len > 0) {
		va0 = PGROUNDDOWN(src_vaddr);
		pa0 = walkaddr(pagetable, va0);
		if (pa0 == 0)
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
 * until a
 * '\0', or max. Return 0 on success, -1 on error.
 * @param pagetable
 * @param dst
 * @param srcva
 * @param max
 * @return int32_t
 */
int32_t copyin_string(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t max)
{
	uint64_t n, va0, pa0;
	int32_t got_null = 0;

	while (got_null == 0 and max > 0) {
		va0 = PGROUNDDOWN(srcva);
		pa0 = walkaddr(pagetable, va0);
		if (pa0 == 0)
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
	return got_null ? 0 : -1;
}

/**
 * @brief Copy from kernel to user. Copy len bytes from src to virtual
 * address dstva in a given page table. And return 0 on success, -1 on
 * error.
 * @param pagetable
 * @param dstva
 * @param src
 * @param len
 * @return int32_t
 */
int32_t copyout(pagetable_t pagetable, void *dstva, void *src, uint64_t len)
{
	uint64_t n, va0, pa0;
	uintptr_t dst_vaddr = (uintptr_t)dstva;

	while (len > 0) {
		va0 = PGROUNDDOWN(dst_vaddr);
		pa0 = walkaddr(pagetable, va0);
		if (pa0 == 0)
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