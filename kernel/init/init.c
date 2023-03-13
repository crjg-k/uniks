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
#include <platform/riscv.h>
#include <process/proc.h>
#include <trap/trap.h>

extern void phymem_init(), kvminit(), kvmenable(), clock_init();
char message[] = "uniks is booting!";


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
	kprintf("\n%s\n", message);
	debugf("stext: %p\tetext: %p", stext, etext);
	debugf("sdata: %p\tedata: %p", sdata, edata);
	debugf("sbss: %p\tebss: %p", sbss, ebss);
	phymem_init();
	kvminit();
	kvmenable();
	// now, in vaddr space!
	proc_init();
	trap_init();
	kputc('\n');
	user_init();
	__sync_synchronize();
	clock_init();
	interrupt_on();
	idle_process();

	panic("will never step here");
}