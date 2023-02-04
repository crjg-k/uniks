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

#include <kstdio.h>
#include <kstring.h>
#include <log.h>
#include <sbi.h>

extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
char message[] = "KOS is running!";
void clean_bss()
{
	memset(sbss, 0, ebss - sbss);
}
void kernel_start(void)
{
	clean_bss();
	kprintf("%s\n\n", message);
	debugf("stext: %p\tetext: %p", stext, etext);
	debugf("sdata: %p\tedata: %p", sdata, edata);
	debugf("sbss: %p\tebss: %p", sbss, ebss);
	panic("ALL DONE");

	kprintf("will never step here");
}