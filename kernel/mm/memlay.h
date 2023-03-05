#ifndef __KERNEL_MM_MEMLAY_H__
#define __KERNEL_MM_MEMLAY_H__

#include "mmu.h"

#define KSTACKPAGE 4			   // number of pages in kernel stack
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)   // sizeof kernel stack
#define PHYSTOP	   (0x80000000 + 128 * 1024 * 1024)   // 128MiB RAM main memory
#define MAXVA	   (1l << (39 - 1))

// map the trampoline page to the highest address,
// in both user and kernel space.
#define TRAMPOLINE (MAXVA - PGSIZE)
// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p)  (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)

#endif /* !__KERNEL_MM_MEMLAY_H__ */
