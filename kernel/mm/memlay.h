#ifndef __KERNEL_MM_MEMLAY_H__
#define __KERNEL_MM_MEMLAY_H__

#include "mmu.h"

#define KSTACKPAGE 4			   // number of pages in kernel stack
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)   // sizeof kernel stack
#define PHYSIZE	   128 * 1024 * 1024
#define RAMSTART   0x80000000
#define PHYSTOP	   (RAMSTART + PHYSIZE)	  // 128MiB RAM main memory
#define MAXVA	   (1l << (39 - 1))

#define PA2ARRAYINDEX(pa)    ((pa - RAMSTART) >> PGSHIFT)
#define ARRAYINDEX2PA(index) ((index << PGSHIFT) + RAMSTART)

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)
// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p)  (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)

#endif /* !__KERNEL_MM_MEMLAY_H__ */
