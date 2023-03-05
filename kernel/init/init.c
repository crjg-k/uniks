/**
 * @file init.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-01-29 15:55:51
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */

#include <defs.h>
#include <kstdio.h>
#include <kstring.h>
#include <log.h>
#include <process/proc.h>
#include <trap/trap.h>

extern void phymem_init(), kvminit(), kvmenable(), clock_init();
extern void *kalloc();
char message[] = "uniks is booting!";

// todo1: read and realize the memory/pagetable free function of vm.c
// todo3: can kerneltrap only save tiny context?

__noreturn __always_inline void idle_process()
{
	while (1) {
		scheduler();
	}
}

void kernel_start()
{
	memset(sbss, 0, ebss - sbss);
	printfinit();
	phymem_init();
	kvminit();
	kvmenable();
	// now, in vaddr space!
	proc_init();
	trap_init();
	kprintf("\n%s\n", message);
	debugf("stext: %p\tetext: %p", stext, etext);
	debugf("sdata: %p\tedata: %p", sdata, edata);
	debugf("sbss: %p\tebss: %p", sbss, ebss);
	kputc('\n');
	user_init();
	__sync_synchronize();
	clock_init();
	interrupt_enable();
	idle_process();

	panic("will never step here");
}