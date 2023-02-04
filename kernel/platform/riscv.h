
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

#endif /* !__KERNEL_PLATFORM_RISCV_H__ */