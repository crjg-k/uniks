#ifndef __KERNEL_MM_MEMLAY_H__
#define __KERNEL_MM_MEMLAY_H__


#include "mmu.h"
#include <uniks/param.h>


#define KSTACKPAGE     (4)   // number of pages in kernel stack
#define KSTACKSIZE     (KSTACKPAGE * PGSIZE)   // sizeof kernel stack
#define PHYMEMSIZE_MiB 128
#define PHYMEMSIZE     (PHYMEMSIZE_MiB * KiB * KiB)
#define RAMSTART       (0x80000000)
#define PHYSTOP	       (RAMSTART + PHYMEMSIZE)	 // 128MiB RAM main memory
#define MAXVA	       (1ll << (39 - 1))

#define PA2ARRAYINDEX(pa)    ((pa - RAMSTART) >> PGSHIFT)
#define ARRAYINDEX2PA(index) ((index << PGSHIFT) + RAMSTART)

/**
 * @brief map the trampoline page to the highest address, in both user and
 * kernel space
 */
#define TRAMPOLINE (MAXVA - PGSIZE)
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)

// user process vaddr space
#define USER_STACK_TOP (TRAPFRAME)


#if (__ASSEMBLER__ == 0)

extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
extern char end[];
extern char kernel_entry[];
extern char KERNEL_BASE_ADDR[];

#endif


#endif /* !__KERNEL_MM_MEMLAY_H__ */