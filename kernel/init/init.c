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
#include <platform/riscv.h>
#include <process/file.h>
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

volatile static int8_t started = 0;

void kernel_start()
{
	if (!r_mhartid()) {
		memset(sbss, 0, ebss - sbss);
		consoleinit();
		printfinit();
		kprintf("\n%s\n", message);
		kprintf("hart %d start\n\n", r_mhartid());
		debugf("stext: %p\tetext: %p", stext, etext);
		debugf("sdata: %p\tedata: %p", sdata, edata);
		debugf("sbss: %p\tebss: %p", sbss, ebss);

		phymem_init();
		kvminit();
		kvmenable();
		// now, in vaddr space!
		proc_init();
		trap_init();
		// plicinit();

		buffer_init();
		inode_init();
		fileinit();
		virtio_disk_init();

		user_init();
		__sync_synchronize();
		clock_init();
		interrupt_on();
		started = 1;
	} else {
		while (!started)
			;
		__sync_synchronize();
		kprintf("hart %d starting\n", r_mhartid());
	}
	idle_process();

	panic("will never step here");
}