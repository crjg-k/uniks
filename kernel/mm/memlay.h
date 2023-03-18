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


// external device
// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0c000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

#endif /* !__KERNEL_MM_MEMLAY_H__ */
