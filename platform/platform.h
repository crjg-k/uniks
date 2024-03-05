#ifndef __KERNEL_PLATFORM_PLATFORM_H__
#define __KERNEL_PLATFORM_PLATFORM_H__

#include <uniks/param.h>


/* === external device === */

#if PLATFORM_QEMU == 1
	/**
	 * @brief qemu provide platform information, copy from
	 * https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c
	 */

	// qemu puts platform-level interrupt controller (PLIC) here.
	#define PLIC		      (0x0C000000LL)
	#define PLIC_PRIORITY	      (PLIC + 0x0)
	#define PLIC_PENDING	      (PLIC + 0x1000)
	#define PLIC_M_ENABLE(hart)   (PLIC + 0x2000 + (hart) * 0x100)
	#define PLIC_S_ENABLE(hart)   (PLIC + 0x2080 + (hart) * 0x100)
	#define PLIC_M_PRIORITY(hart) (PLIC + 0x200000 + (hart) * 0x2000)
	#define PLIC_S_PRIORITY(hart) (PLIC + 0x201000 + (hart) * 0x2000)
	#define PLIC_M_CLAIM(hart)    (PLIC + 0x200004 + (hart) * 0x2000)
	#define PLIC_S_CLAIM(hart)    (PLIC + 0x201004 + (hart) * 0x2000)

	// qemu puts UART registers here in physical memory.
	#define UART0 (0x10000000LL)

	// virtio mmio interface
	#define VIRTIO0 (0x10001000)


/**
 * @brief qemu provide platform information, copy from
 * https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h
 */
enum {
	UART0_IRQ = 10,
	RTC_IRQ = 11,
	VIRTIO_IRQ = 1, /* 1 to 8 */
	VIRTIO_COUNT = 8,
	PCIE_IRQ = 0x20,	    /* 32 to 35 */
	VIRT_PLATFORM_BUS_IRQ = 64, /* 64 to 95 */
};

	#define MAX_INTTERUPT_SRC_ID (36)

#endif


#endif /* !__KERNEL_PLATFORM_PLATFORM_H__ */