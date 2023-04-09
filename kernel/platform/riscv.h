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


#define PRILEVEL_U 0
#define PRILEVEL_S 1
#define PRILEVEL_M 3

// trap->interrupt
#define IRQ_U_SOFT	       0
#define IRQ_S_SOFT	       1
#define IRQ_U_TIMER	       4
#define IRQ_S_TIMER	       5
#define IRQ_U_EXT	       8
#define IRQ_S_EXT	       9
// trap->exception
#define EXC_INST_ADDR_MISALIGN 0
#define EXC_INST_ACCESSFAULT   1
#define EXC_INST_ILLEGAL       2
#define EXC_BREAK	       3
#define EXC_LD_ADDR_MISALIGN   4
#define EXC_LD_ACCESSFAULT     5
#define EXC_SD_ADDR_MISALIGN   6
#define EXC_SD_ACCESSFAULT     7
#define EXC_U_ECALL	       8
#define EXC_S_ECALL	       9
#define EXC_INST_PAGEFAULT     12
#define EXC_LD_PAGEFAULT       13
#define EXC_SD_PAGEFAULT       15

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
#define PTENUM		     PGSIZE / 8
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> 12))
#define w_satp(x)	     ({ asm volatile("csrw satp, %0" : : "r"(x)); })
#define PA2PTE(pa)	     ((((uint64_t)pa) >> 12) << 10)   // pysical addr to pte
#define PTE2PA(pte)	     (((pte) >> 10) << 12)   // pte to pysical addr
#define PXSHIFT(level)	     (PGSHIFT + (9 * (level)))
#define PX(level, va)	     ((((uint64_t)(va)) >> PXSHIFT(level)) & 0x1ff)
#define PTE_FLAGS(pte)	     ((pte)&0x3FF)


#define r_mhartid() \
	({ \
		uint64_t __tmp; \
		asm volatile("mv %0, tp" : "=r"(__tmp)); \
		__tmp; \
	})
// enable device interrupts
static __always_inline void interrupt_on()
{
	set_csr(sstatus, SSTATUS_SIE);
}

// disable device interrupts
static __always_inline void interrupt_off()
{
	clear_csr(sstatus, SSTATUS_SIE);
}

// are device interrupts enabled?
static __always_inline uint64_t interrupt_get()
{
	return read_csr(sstatus);
}

// set device interrupts enabled status to a specific value
static __always_inline void interrupt_set(uint64_t val)
{
	write_csr(sstatus, val);
}

// zero, zero means flush all TLB entries
#define sfence_vma() ({ asm volatile("sfence.vma zero, zero\n\tnop"); })


#endif /* !__KERNEL_PLATFORM_RISCV_H__ */