#include "plic.h"
#include <device/device.h>
#include <device/uart.h>
#include <platform/platform.h>
#include <platform/riscv.h>


void plicinit()
{
	// set desired IRQ priorities non-zero (otherwise disabled).
	*(uint32_t *)(PLIC + UART0_IRQ * sizeof(uint32_t)) = 1;
	*(uint32_t *)(PLIC + VIRTIO_IRQ * sizeof(uint32_t)) = 1;
}

void plicinithart()
{
	int32_t hart = r_mhartid();

	/**
	 * @brief set enable bits for this hart's S-mode for the uart and virtio
	 * disk.
	 */
	*(uint32_t *)PLIC_S_ENABLE(hart) = (1 << UART0_IRQ) | (1 << VIRTIO_IRQ);

	/**
	 * @brief set this hart's S-mode priority threshold to 0. namely accept
	 * all interrupt
	 */
	*(uint32_t *)PLIC_S_PRIORITY(hart) = 0;
}

/**
 * @brief Writing the interrupt ID it received from the claim (irq) to the
 * complete register would signal the PLIC we've served this IRQ. The PLIC does
 * not check whether the completion ID is the same as the last claim ID for that
 * target. If the completion ID does not match an interrupt source that is
 * currently enabled for the target, the completion is silently ignored.
 * @return int32_t
 */
__always_inline int32_t plic_claim()
{
	int32_t hart = r_mhartid();
	int32_t irq = *(int32_t *)PLIC_S_CLAIM(hart);
	return irq;
}

/**
 * @brief Writing the interrupt ID it received from the claim (irq) to the
 * complete register would signal the PLIC we've served this IRQ. The PLIC does
 * not check whether the completion ID is the same as the last claim ID for that
 * target. If the completion ID does not match an interrupt source that is
 * currently enabled for the target, the completion is silently ignored.
 * @param irq
 */
__always_inline void plic_complete(int32_t irq)
{
	int32_t hart = r_mhartid();
	*(uint32_t *)PLIC_S_CLAIM(hart) = irq;
}

void external_interrupt_handler()
{
	int32_t irq = plic_claim();
	device_interrupt_handler(irq);
	plic_complete(irq);
}