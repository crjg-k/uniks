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
#include <trap/trap.h>

extern void phymem_init(), kvminit(), kvminithart(), clock_init();
extern void *kalloc();
char message[] = "KOS is running!";

void kernel_start()
{
	memset(sbss, 0, ebss - sbss);
	phymem_init();
	kvminit();
	kvminithart();
	// now, in vaddr space!
	trap_init();
	clock_init();
	interrupt_enable();
	kputc('\n');
	kprintf("%s\n", message);
	debugf("stext: %p\tetext: %p", stext, etext);
	debugf("sdata: %p\tedata: %p", sdata, edata);
	debugf("sbss: %p\tebss: %p", sbss, ebss);
	kputc('\n');
	while (1) /* waiting for interrupt */
		;

	panic("will never step here");
}