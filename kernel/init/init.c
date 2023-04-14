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
#include <driver/console.h>
#include <driver/virtio_disk.h>
#include <fs/fs.h>
#include <kstdio.h>
#include <kstring.h>
#include <log.h>
#include <mm/blkbuffer.h>
#include <mm/memlay.h>
#include <platform/riscv.h>
#include <process/file.h>
#include <process/proc.h>
#include <trap/trap.h>

extern void phymem_init(), kvminit(), kvmenable(), clock_init();
char message[] = "uniks boot over!";
extern int32_t freepagenum;
volatile static int8_t started = 0;


__noreturn __always_inline void idle_process()
{
	while (1) {
		scheduler();
	}
}

void kernel_start()
{
	if (!r_mhartid()) {
		memset(sbss, 0, ebss - sbss);
		consoleinit();
		printfinit();
		debugf("stext: %p\tetext: %p", stext, etext);
		debugf("sdata: %p\tedata: %p", sdata, edata);
		debugf("sbss: %p\tebss: %p", sbss, ebss);
		debugf("end: %p\n", end);

		phymem_init();
		kprintf("==> total memory size: %l MiB, total pages: %d, free pages: %d\n",
			PHYSIZE / 1024 / 1024, PHYSIZE / 4096, freepagenum);
		kvminit();
		kvmenable();
		// now, in vaddr space!
		proc_init();
		trap_init();
		// plicinit();

		// hint: below 4 init functions maybe could let other harts do
		buffer_init();
		inode_init();
		fileinit();
		virtio_disk_init();

		user_init(1);
		__sync_synchronize();
		clock_init();

		kprintf("\n%s\n", message);
		kprintf("hart %d start\n\n", r_mhartid());

		interrupt_on();
		started = 1;
	} else {
		while (!started)
			;
		__sync_synchronize();
		kprintf("hart %d starting\n", r_mhartid());
	}
	idle_process();

	panic("will never step here!");
}