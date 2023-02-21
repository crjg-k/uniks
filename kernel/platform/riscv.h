#ifndef __KERNEL_PLATFORM_RISCV_H__
#define __KERNEL_PLATFORM_RISCV_H__

#include <defs.h>

#define read_csr(reg) \
	({ \
		uint64_t __tmp; \
		asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
		__tmp; \
	})

#define write_csr(reg, val) \
	({ \
		if (__builtin_constant_p(val) and (uint64_t)(val) < 32) \
			asm volatile("csrw " #reg ", %0" ::"i"(val)); \
		else \
			asm volatile("csrw " #reg ", %0" ::"r"(val)); \
	})

#define set_csr(reg, bit) \
	({ \
		uint64_t __tmp; \
		if (__builtin_constant_p(bit) and (uint64_t)(bit) < 32) \
			asm volatile("csrrs %0, " #reg ", %1" \
				     : "=r"(__tmp) \
				     : "i"(bit)); \
		else \
			asm volatile("csrrs %0, " #reg ", %1" \
				     : "=r"(__tmp) \
				     : "r"(bit)); \
		__tmp; \
	})

#define clear_csr(reg, bit) \
	({ \
		uint64_t __tmp; \
		if (__builtin_constant_p(bit) and (uint64_t)(bit) < 32) \
			asm volatile("csrrc %0, " #reg ", %1" \
				     : "=r"(__tmp) \
				     : "i"(bit)); \
		else \
			asm volatile("csrrc %0, " #reg ", %1" \
				     : "=r"(__tmp) \
				     : "r"(bit)); \
		__tmp; \
	})


__always_inline void sfence_vma()
{
	// zero, zero means flush all TLB entries
	asm volatile("sfence.vma zero, zero\n\tnop");
}


#define IRQ_U_SOFT  0
#define IRQ_S_SOFT  1
#define IRQ_U_TIMER 4
#define IRQ_S_TIMER 5
#define IRQ_U_EXT   8
#define IRQ_S_EXT   9

/* trap and interrupt related */
#define MIP_SSIP (1 << IRQ_S_SOFT)
#define MIP_STIP (1 << IRQ_S_TIMER)
#define MIP_SEIP (1 << IRQ_S_EXT)

#define SSTATUS_UIE  0x00000001
#define SSTATUS_SIE  0x00000002
#define SSTATUS_UPIE 0x00000010
#define SSTATUS_SPIE 0x00000020
#define SSTATUS_SPP  0x00000100

#define PTE_V		     (1 << 0)
#define PTE_R		     (1 << 1)
#define PTE_W		     (1 << 2)
#define PTE_X		     (1 << 3)
#define PTE_U		     (1 << 4)
#define SATP_SV39	     (8l << 60)	  // RISCV Sv39 page table scheme
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> 12))
#define w_satp(x)	     ({ asm volatile("csrw satp, %0" : : "r"(x)); })
#define PA2PTE(pa)	     ((((uint64_t)pa) >> 12) << 10)   // pysical addr to pte
#define PTE2PA(pte)	     (((pte) >> 10) << 12)   // pte to pysical addr
#define PXSHIFT(level)	     (PGSHIFT + (9 * (level)))
#define PX(level, va)	     ((((uint64_t)(va)) >> PXSHIFT(level)) & 0x1ff)
#define MAXVA		     (1l << (39 - 1))	// Sv39

#endif /* !__KERNEL_PLATFORM_RISCV_H__ */