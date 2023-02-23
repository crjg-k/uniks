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
#include <process/proc.h>
#include <trap/trap.h>

extern void phymem_init(), kvminit(), kvmenable(), clock_init();
extern void *kalloc();
char message[] = "uniks is running!";

#include <driver/clock.h>
void idle_process()
{
	// int32_t i = 0;
	while (1) {
		// kprintf("idle=>%d\n", i++);
	}
}

void kernel_start()
{
	memset(sbss, 0, ebss - sbss);
	phymem_init();
	kvminit();
	kvmenable();
	// now, in vaddr space!
	proc_init();
	trap_init();
	clock_init();
	kputc('\n');
	kprintf("%s\n", message);
	debugf("stext: %p\tetext: %p", stext, etext);
	debugf("sdata: %p\tedata: %p", sdata, edata);
	debugf("sbss: %p\tebss: %p", sbss, ebss);
	kputc('\n');
	interrupt_enable();
	idle_process();

	panic("will never step here");
}