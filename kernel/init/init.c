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

#include <driver/clock.h>
#include <kstdio.h>
#include <kstring.h>
#include <log.h>
#include <trap/trap.h>

extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
char message[] = "KOS is running!";
void clean_bss()
{
	memset(sbss, 0, ebss - sbss);
}
void kernel_start()
{
	clean_bss();
	kputc('\n');
	trap_init();
	clock_init();
	interrupt_enable();
	kprintf("%s\n", message);
	debugf("stext: %p\tetext: %p", stext, etext);
	debugf("sdata: %p\tedata: %p", sdata, edata);
	debugf("sbss: %p\tebss: %p", sbss, ebss);
	kputc('\n');
	for (int8_t i = 0;; i++) /* waiting for interrupt */
		/*kprintf("i = %d\n", i)*/;

	panic("ALL DONE");
	kprintf("will never step here");
}