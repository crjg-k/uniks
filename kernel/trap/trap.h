#ifndef __KERNEL_TRAP_TRAP_H__
#define __KERNEL_TRAP_TRAP_H__

#include <defs.h>


void trap_init();
uint64_t trap_handler(uint64_t epc, uint64_t cause);
void interrupt_enable();

#endif /* !__KERNEL_TRAP_TRAP_H__ */