#include "memlay.h"
#include "mmu.h"
#include <defs.h>
#include <kassert.h>
#include <kstring.h>
#include <platform/riscv.h>
#include <process/proc.h>

pagetable_t kernel_pagetable;

extern char trampoline[];
extern void *kalloc();
extern void kfree(void *);

void kvmenable()
{
	// wait for any previous writes to the page table memory to finish.
	sfence_vma();
	w_satp(MAKE_SATP(kernel_pagetable));
	// flush stale entries from the TLB.
	sfence_vma();
}

/**
 * @brief Return the address of the PTE in page table pagetable that corresponds
 * to virtual address va. If alloc!=0, create any required page-table pages.
 *
 * @param pagetable
 * @param va
 * @param alloc wether allow to allocate memory
 * @return pte_t*
 */
pte_t *walk(pagetable_t pagetable, uint64_t va, int32_t alloc)
{
	assert(va < MAXVA);

	for (int32_t level = 2; level > 0; level--) {
		pte_t *pte = &pagetable[PX(level, va)];
		if (*pte & PTE_V) {
			pagetable = (pagetable_t)PTE2PA(*pte);
		} else {   // this page is not valid
			if (!alloc or (pagetable = (pde_t *)kalloc()) == 0)
				return 0;
			memset(pagetable, 0, PGSIZE);
			*pte = PA2PTE(pagetable) | PTE_V;
		}
	}
	return &pagetable[PX(0, va)];
}

// look up a vaddr in user space, return the paddr, or 0 if not mapped
uint64_t walkaddr(pagetable_t pagetable, uint64_t va)
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

int32_t mappages(pagetable_t pagetable, uintptr_t va, uintptr_t size,
		 uintptr_t pa, int32_t perm)
{
	assert(size != 0);

	uint64_t a = PGROUNDDOWN(va), last = PGROUNDUP(va + size - 1);
	pte_t *pte;
	while (a < last) {
		if ((pte = walk(pagetable, a, 1)) == 0)
			return -1;
		assert(!(*pte & PTE_V));
		*pte = PA2PTE(pa) | perm | PTE_V;
		a += PGSIZE;
		pa += PGSIZE;
	}
	return 0;
}

static pagetable_t kvmmake()
{
	pagetable_t kpgtbl;
	kpgtbl = (pagetable_t)kalloc();
	memset(kpgtbl, 0, PGSIZE);
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

	proc_mapstacks(kpgtbl);

	return kpgtbl;
}

// only initialize the kernel pagetable when booting
void kvminit()
{
	kernel_pagetable = kvmmake();
}

// load the user/initcode into address 0 of pagetable
void uvmfirst(pagetable_t pagetable, uint32_t *src, uint32_t sz)
{
	assert(sz < PGSIZE);
	char *mem = kalloc();
	memset(mem, 0, PGSIZE);
	mappages(pagetable, 0, PGSIZE, (uint64_t)mem,
		 PTE_W | PTE_R | PTE_X | PTE_U);
	memcpy(mem, src, sz);
}

// create an empty user page table and returns 0 if out of memory
pagetable_t uvmcreate()
{
	pagetable_t pagetable;
	pagetable = (pagetable_t)kalloc();
	if (pagetable == 0)
		return 0;
	memset(pagetable, 0, PGSIZE);
	return pagetable;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void freewalk(pagetable_t pagetable)
{
	for (int32_t i = 0; i < PTENUM; i++) {
		pte_t pte = pagetable[i];
		if ((pte & PTE_V) and (pte & (PTE_R | PTE_W | PTE_X)) == 0) {
			// this PTE points to a lower-level page table.
			uint64_t child = PTE2PA(pte);
			freewalk((pagetable_t)child);
			pagetable[i] = 0;
		} else if (pte & PTE_V) {
			panic("freewalk: leaf");
		}
	}
	kfree((void *)pagetable);
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void uvmunmap(pagetable_t pagetable, uint64_t va, uint64_t npages,
	      int32_t do_free)
{
	assert((va % PGSIZE) == 0);
	pte_t *pte;
	for (uint64_t a = va; a < va + npages * PGSIZE; a += PGSIZE) {
		if ((pte = walk(pagetable, a, 0)) == 0)
			panic("uvmunmap: walk");
		if ((*pte & PTE_V) == 0)
			panic("uvmunmap: not mapped");
		if (PTE_FLAGS(*pte) == PTE_V)
			panic("uvmunmap: not a leaf");
		if (do_free) {
			uint64_t pa = PTE2PA(*pte);
			kfree((void *)pa);
		}
		*pte = 0;
	}
}

// free user memory pages, then free page-table pages
void uvmfree(pagetable_t pagetable, uint64_t sz)
{
	if (sz > 0)
		uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);
	freewalk(pagetable);
}

// receive a parent process's page table, and copy its page table both memory
// content into child's address space
int64_t uvmcopy(pagetable_t old, pagetable_t new, uint64_t sz)
{
	pte_t *pte;
	uint64_t pa, i;
	uint32_t flags;
	char *mem;

	for (i = 0; i < sz; i += PGSIZE) {
		if ((pte = walk(old, i, 0)) == 0)
			panic("uvmcopy: pte should exist");
		if ((*pte & PTE_V) == 0)
			panic("uvmcopy: page not present");
		pa = PTE2PA(*pte);
		flags = PTE_FLAGS(*pte);
		if ((mem = kalloc()) == 0)
			goto err;
		memcpy(mem, (char *)pa, PGSIZE);
		if (mappages(new, i, PGSIZE, (uint64_t)mem, flags) != 0) {
			kfree(mem);
			goto err;
		}
	}
	return 0;

err:
	uvmunmap(new, 0, i / PGSIZE, 1);
	return -1;
}


/* transmit data between user space and kernel space */

// copy from user to kernel, return 0 on success, -1 on error
int64_t copyin(pagetable_t pagetable, char *dst, uint64_t srcva, uint64_t len)
{
	uint64_t n, va0, pa0;

	while (len > 0) {
		va0 = PGROUNDDOWN(srcva);
		pa0 = walkaddr(pagetable, va0);
		if (!pa0)
			return -1;
		n = PGSIZE - (srcva - va0);
		if (n > len)
			n = len;
		memcpy(dst, (void *)(pa0 + (srcva - va0)), n);

		len -= n;
		dst += n;
		srcva = va0 + PGSIZE;
	}
	return 0;
}