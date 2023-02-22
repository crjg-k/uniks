#include "memlay.h"
#include "mmu.h"
#include <defs.h>
#include <kstring.h>
#include <platform/riscv.h>

pagetable_t kernel_pagetable;

extern void *kalloc();

void kvminithart()
{
	// wait for any previous writes to the page table memory to finish.
	sfence_vma();
	w_satp(MAKE_SATP(kernel_pagetable));
	// flush stale entries from the TLB.
	sfence_vma();
}

/**
 * @brief
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
	mappages(kpgtbl, KERNELBASE, (uint64_t)(etext - KERNELBASE), KERNELBASE,
		 PTE_R | PTE_X);
	// map kernel data segment(include stack) and the remainder physical RAM
	mappages(kpgtbl, (uint64_t)etext, PHYSTOP - (uint64_t)etext,
		 (uint64_t)etext, PTE_R | PTE_W);

	return kpgtbl;
}

/* only initialize the kernel pagetable when booting */
void kvminit()
{
	kernel_pagetable = kvmmake();
}