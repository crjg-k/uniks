#include "vm.h"
#include "memlay.h"
#include "mmu.h"
#include <mm/phys.h>
#include <platform/platform.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <uniks/defs.h>
#include <uniks/errno.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>
#include <uniks/param.h>


pagetable_t kernel_pagetable;
int32_t mem_map[PHYMEMSIZE >> PGSHIFT];


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
pte_t *walk(pagetable_t pagetable, uint64_t va, int32_t alloc)
{
	assert(va < MAXVA);

	for (int32_t level = 2; level > 0; level--) {
		pte_t *pte = &pagetable[PX(level, va)];
		if (*pte & PTE_V) {
			pagetable = (pagetable_t)PTE2PA(*pte);
		} else {   // this page is not valid
			if (!alloc or
			    (pagetable = (pde_t *)pages_alloc(1)) == 0)
				return NULL;
			memset(pagetable, 0, PGSIZE);
			*pte = PA2PTE(pagetable) | PTE_V;
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
uintptr_t walkaddr(pagetable_t pagetable, uintptr_t va)
{
	pte_t *pte = walk(pagetable, va, 0);
	if (pte == 0)
		return 0;
	if ((*pte & PTE_V) == 0)
		return 0;
	if ((*pte & PTE_U) == 0)
		return 0;
	return PTE2PA(*pte);
}

uintptr_t vaddr2paddr(pagetable_t pagetable, uintptr_t va)
{
	uintptr_t pgstart = walkaddr(pagetable, va);
	if (pgstart == 0)
		return 0;
	return OFFSETPAGE(va) + pgstart;
}

int32_t mappages(pagetable_t pagetable, uintptr_t va, size_t size, uintptr_t pa,
		 int32_t perm, int8_t recordref)
{
	assert(size != 0);

	uint64_t a = PGROUNDDOWN(va), last = PGROUNDUP(va + size - 1);
	pte_t *pte;
	while (a < last) {
		// potential optimization: deeper interweaving with walk()
		if ((pte = walk(pagetable, a, 1)) == NULL)
			return -1;
		*pte = PA2PTE(pa) | perm | PTE_V;
		if (recordref)
			mem_map[PA2ARRAYINDEX(pa)]++;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

static pagetable_t kvmmake()
{
	pagetable_t kpgtbl;
	kpgtbl = (pagetable_t)pages_alloc(1);
	memset(kpgtbl, 0, PGSIZE);
	// uart registers
	assert(mappages(kpgtbl, UART0, PGSIZE, UART0, PTE_R | PTE_W, 0) != -1);
	// virtio mmio disk interface
	assert(mappages(kpgtbl, VIRTIO0, PGSIZE, VIRTIO0, PTE_R | PTE_W, 0) !=
	       -1);
	// PLIC
	assert(mappages(kpgtbl, PLIC, 0x400000, PLIC, PTE_R | PTE_W, 0) != -1);

#define KERNELBASE (uintptr_t) KERNEL_BASE_ADDR
	// map kernel text segment
	assert(mappages(kpgtbl, KERNELBASE, (uint64_t)(etext - KERNELBASE),
			KERNELBASE, PTE_R | PTE_X, 0) != -1);
	// map kernel data segment(include stack) and the remainder physical RAM
	assert(mappages(kpgtbl, (uint64_t)etext, PHYSTOP - (uint64_t)etext,
			(uint64_t)etext, PTE_R | PTE_W, 0) != -1);
	// map the trampoline to the highest virtual address in the kernel
	assert(mappages(kpgtbl, TRAMPOLINE, PGSIZE, (uint64_t)trampoline,
			PTE_R | PTE_X, 0) != -1);

	return kpgtbl;
}

// only initialize the kernel pagetable when booting
void kvminit()
{
	kernel_pagetable = kvmmake();
}

// load the user/initcode into address USER_TEXT_START of pagetable
void uvmfirst(pagetable_t pagetable, uint32_t *src, uint32_t sz)
{
	char *pgstart = pages_alloc(1);
	mappages(pagetable, USER_TEXT_START, PGSIZE, (uintptr_t)pgstart,
		 PTE_R | PTE_X | PTE_U, 1);
	memcpy(pgstart, src, sz);
}

// create an empty user page table and returns 0 if out of memory
pagetable_t uvmcreate()
{
	pagetable_t pagetable;
	pagetable = (pagetable_t)pages_alloc(1);
	if (pagetable == 0)
		return 0;
	memset(pagetable, 0, PGSIZE);
	return pagetable;
}

/**
 * @brief free pagetable and all leaf must already have been removed
 *
 * @param pagetable
 */
void freewalk(pagetable_t pagetable)
{
	for (int32_t i = 0; i < PTENUM; i++) {
		pte_t pte = pagetable[i];

		// means that this pte has child level
		assert(!(pte & PTE_V) or (pte & (PTE_R | PTE_W | PTE_X)));

		uint64_t child = PTE2PA(pte);
		freewalk((pagetable_t)child);
		pagetable[i] = 0;
	}
	pages_free((void *)pagetable);
}

/**
 * @brief remove n pages of pagetable starting from va and free the phymem
 * optionally
 *
 * @param pagetable
 * @param va must be page-aligned namely 4096 bytes aligned
 * @param npages
 * @param do_free
 */
void uvmunmap(pagetable_t pagetable, uintptr_t va, uintptr_t npages,
	      int32_t do_free)
{
	assert((va % PGSIZE) == 0);

	pte_t *pte;
	for (uint64_t a = va; a < va + npages * PGSIZE; a += PGSIZE) {
		assert((pte = walk(pagetable, a, 0)) != NULL);
		assert((*pte & PTE_V) != 0);	    // assert mappings is exist
		assert(PTE_FLAGS(*pte) != PTE_V);   // assert it is a leaf
		if (do_free) {
			uint64_t pa = PTE2PA(*pte);
			pages_free((void *)pa);
		}
		*pte = 0;   // do unmap operation by clear valid flag
	}
}

// free user memory pages, then free page-table pages
void uvmfree(pagetable_t pagetable, uint64_t sz)
{
	if (sz > 0)
		uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);
	freewalk(pagetable);
}

/**
 * @brief receive a parent process's page table, and copy its page table only
 * into child's address space (since the COW is implemented)
 *
 * @param old namely parent process
 * @param new namely child process
 * @param sz
 * @return int64_t
 */
int64_t uvmcopy(pagetable_t old, pagetable_t new, uint64_t sz)
{
	// note: COW mechanism is not applicable to kstack page!!
	pte_t *pte;
	uint64_t pa, i;
	uint32_t flags;

	for (i = 0; i < sz; i += PGSIZE) {
		assert((pte = walk(old, i, 0)) != NULL);
		assert((*pte & PTE_V) != 0);
		pa = PTE2PA(*pte);
		clear_variable_bit(*pte,
				   PTE_W);   // clear the write permission bit
		flags = PTE_FLAGS(*pte);
		// add mapping to childproc pagetable
		if (mappages(new, i, PGSIZE, (uint64_t)pa, flags, 1) != 0) {
			goto err;
		}
		mem_map[PA2ARRAYINDEX(pa)]++;
		// note: must must must flush the TLB to validate the
		// note: modification of the page table
		invalidate(i);
	}
	return 0;

err:
	uvmunmap(new, 0, i / PGSIZE, 1);
	return -1;
}

int32_t un_wp_page(pte_t *pte)
{
	uint64_t pa = PTE2PA(*pte);
	assert(mem_map[PA2ARRAYINDEX(pa)] >= 1);
	// if it is referred one time, only add write permission
	if (mem_map[PA2ARRAYINDEX(pa)] == 1) {
		set_variable_bit(*pte, PTE_W);
		return 0;
	}
	// else duplicate page content
	char *mem;
	if ((mem = pages_alloc(1)) == 0)
		goto err;
	memcpy(mem, (char *)pa, PGSIZE);
	register uint64_t perm = PTE_FLAGS(*pte);
	*pte = PA2PTE(mem) | PTE_W | perm;
	mem_map[PA2ARRAYINDEX(pa)]--;
	return 0;
err:
	return -ENOMEM;
}

// tackle cow mechanism
__always_inline void do_wp_page(pte_t *pte, uintptr_t va)
{
	// hint: don't cope with the oom situation
	un_wp_page(pte);
	// note: must flush the TLB after modifying page table
	invalidate(va);
}

// lazy load ELF page from secondary storage
void do_no_page() {}
void do_inst_page_fault(uintptr_t fault_vaddr) {}
void do_ld_page_fault(uintptr_t fault_vaddr) {}
void do_sd_page_fault(uintptr_t fault_vaddr)
{
	struct proc_t *p = myproc();
	pte_t *pte = walk(p->pagetable, fault_vaddr, 0);
	assert(pte != NULL);
	if (!PAGE_ISVALID(*pte)) {
		do_no_page();
	} else if (PAGE_ISWRITABLE(*pte)) {
		return;
	} else {
		do_wp_page(pte, fault_vaddr);
	}
	assert(p->magic == UNIKS_MAGIC);
}

/**
 * @brief page writing verify, and if the page can't be written resulting from
 * the COW mechanism, then copy it
 * @param address must be aligned to page
 */
__always_inline void write_verify(uintptr_t address)
{
	do_sd_page_fault(address);
}

/**
 * @brief verify function before writing to user space, and the names of this
 * series of functions come from Linux-0.11
 * @param addr
 * @param size
 */
void verify_area(void *addr, int64_t size)
{
	uintptr_t start = (uintptr_t)addr;
	size = OFFSETPAGE(size);
	start = PGROUNDDOWN(start);

	while (size > 0) {
		size -= PGSIZE;
		write_verify(start);
		start += PGSIZE;
	}
}

/* === transmit data between user space and kernel space === */

/**
 * @brief Copy from kernel to user. Copy len bytes to dst from virtual address
 * srcva in a given page table. And return 0 on success, -1 on error
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
 * Copy bytes to dst from virtual address srcva in a given page table, until a
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
 * @brief Copy from kernel to user. Copy len bytes from src to virtual address
 * dstva in a given page table. And return 0 on success, -1 on error.
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