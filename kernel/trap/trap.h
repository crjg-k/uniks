#ifndef __KERNEL_TRAP_TRAP_H__
#define __KERNEL_TRAP_TRAP_H__


#include <uniks/defs.h>


extern char *fault_msg[];


void trap_inithart();
void trap_init();
void usertrap_handler();
void usertrapret();


#endif /* !__KERNEL_TRAP_TRAP_H__ */