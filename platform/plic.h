#ifndef __KERNEL_PLATFORM_PLIC_H__
#define __KERNEL_PLATFORM_PLIC_H__

#include <uniks/defs.h>


void plicinit();
void plicinithart();
int32_t plic_claim();
void plic_complete(int32_t irq);
void external_interrupt_handler();


#endif