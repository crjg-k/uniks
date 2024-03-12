#ifndef __KERNEL_MM_MEMLAY_H__
#define __KERNEL_MM_MEMLAY_H__


#include "mmu.h"

#define KSTACKPAGE     (4)   // number of pages in kernel stack
#define KSTACKSIZE     (KSTACKPAGE * PGSIZE)   // sizeof kernel stack
#define PHYMEMSIZE_MiB 128
#define PHYMEMSIZE     (PHYMEMSIZE_MiB * 1024 * 1024)
#define RAMSTART       (0x80000000)
#define PHYSTOP	       (RAMSTART + PHYMEMSIZE)	 // 128MiB RAM main memory
#define MAXVA	       (1LL << (39 - 1))

#define PA2ARRAYINDEX(pa)    ((pa - RAMSTART) >> PGSHIFT)
#define ARRAYINDEX2PA(index) ((index << PGSHIFT) + RAMSTART)

/**
 * @brief map the trampoline page to the highest address, in both user and
 * kernel space
 */
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)

// user process vaddr space
#define USER_STACK_TOP			    (TRAPFRAME)


#endif /* !__KERNEL_MM_MEMLAY_H__ */