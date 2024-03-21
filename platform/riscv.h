#ifndef __KERNEL_PLATFORM_RISCV_H__
#define __KERNEL_PLATFORM_RISCV_H__


#include <mm/mmu.h>
#include <uniks/defs.h>


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


#define PRILEVEL_U (0)
#define PRILEVEL_S (1)
#define PRILEVEL_M (3)

// trap->interrupt
#define IRQ_S_SOFT	       (1)
#define IRQ_S_TIMER	       (5)
#define IRQ_S_EXT	       (9)
// trap->exception
#define EXC_INST_ADDR_MISALIGN (0)
#define EXC_INST_ACCESSFAULT   (1)
#define EXC_INST_ILLEGAL       (2)
#define EXC_BREAKPOINT	       (3)
#define EXC_LD_ADDR_MISALIGN   (4)
#define EXC_LD_ACCESSFAULT     (5)
#define EXC_SD_ADDR_MISALIGN   (6)
#define EXC_SD_ACCESSFAULT     (7)
#define EXC_U_ECALL	       (8)
#define EXC_S_ECALL	       (9)
#define EXC_INST_PAGEFAULT     (12)
#define EXC_LD_PAGEFAULT       (13)
#define EXC_SD_PAGEFAULT       (15)

/* trap and interrupt related */
#define SIE_SSIE (1 << IRQ_S_SOFT)
#define SIE_STIE (1 << IRQ_S_TIMER)
#define SIE_SEIE (1 << IRQ_S_EXT)

#define SSTATUS_UIE  (0x00000001)
#define SSTATUS_SIE  (0x00000002)
#define SSTATUS_UPIE (0x00000010)
#define SSTATUS_SPIE (0x00000020)
#define SSTATUS_SPP  (0x00000100)


struct pgtable_entry_t {
	union {
		struct {
			uint8_t valid : 1;
			uint8_t read : 1;
			uint8_t write : 1;
			uint8_t execute : 1;
			uint8_t user : 1;	// user space available?
			uint8_t global : 1;	// is global available?
			uint8_t accessed : 1;	// has been accessed?
			uint8_t dirty : 1;	// is dirty?
		};
		uint8_t perm : 8;
	};

	union {
		struct {
			uint8_t unrelease : 1;
			uint8_t RSW : 1;
			uintptr_t ppn0 : 9;
			uintptr_t ppn1 : 9;
			uintptr_t ppn2 : 26;
		};
		struct {
			uint8_t : 1;
			uint8_t : 1;
			uintptr_t paddr : 44;
		};
	};

	uint32_t reserved : 64 - 55;   // need to always be 0
} __packed;


#define PTE_V		     (1 << 0)
#define PTE_R		     (1 << 1)
#define PTE_W		     (1 << 2)
#define PTE_X		     (1 << 3)
#define PTE_U		     (1 << 4)
#define SATP_SV39	     (8ll << 60)   // RISCV Sv39 page table scheme
#define PTENUM		     (PGSIZE / sizeof(pte_t))
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64_t)pagetable) >> PGSHIFT))
#define W_SATP(x)	     ({ asm volatile("csrw satp, %0" : : "r"(x)); })
#define PA2PTE(pa)	     ((((uint64_t)pa) >> PGSHIFT) << 10)   // pysical addr to pte
#define PNO2PA(pa)	     (uintptr_t)((pa) << PGSHIFT)
#define PXSHIFT(level)	     (PGSHIFT + (9 * (level)))
#define PX(level, va)	     ((((uint64_t)(va)) >> PXSHIFT(level)) & 0x1ff)
#define PTE_FLAGS(pte)	     get_var_bit(*(uint64_t *)pte, 0x3ff)


#define cpuid() \
	({ \
		uint32_t __tmp; \
		asm volatile("csrr t6, sscratch\n\t" \
			     "lwu %0, 0(t6)" \
			     : "=r"(__tmp) \
			     : \
			     : "t6"); \
		__tmp; \
	})

// enable device interrupts
__always_inline void interrupt_on()
{
	set_csr(sstatus, SSTATUS_SIE);
}

// disable device interrupts
__always_inline void interrupt_off()
{
	clear_csr(sstatus, SSTATUS_SIE);
}

// are device interrupts enabled?
__always_inline uint64_t interrupt_get()
{
	return read_csr(sstatus);
}

// set device interrupts enabled status to a specific value
__always_inline void interrupt_set(uint64_t val)
{
	write_csr(sstatus, val);
}

__always_inline void external_interrupt_enable()
{
	set_csr(sie, SIE_SEIE);	  // enable external interrupt in sie
}

__always_inline void timer_interrupt_enable()
{
	set_csr(sie, SIE_STIE);	  // enable timer interrupt in sie
}

__always_inline void software_interrupt_enable()
{
	set_csr(sie, SIE_SSIE);	  // enable software interrupt in sie
}

__always_inline void all_interrupt_enable()
{
	set_csr(sie,
		SIE_SEIE | SIE_STIE |
			SIE_SSIE);   // enable all kinds of interrupt in sie
}

// zero, zero means flush all TLB entries
#define sfence_vma() ({ asm volatile("sfence.vma zero, zero\n\tnop"); })

__always_inline void invalidate(uintptr_t va)
{
	asm volatile("sfence.vma %0, zero\n\tnop" : "=r"(va));
}


extern int32_t boothartid;


#endif /* !__KERNEL_PLATFORM_RISCV_H__ */