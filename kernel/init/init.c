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

#include <device/device.h>
#include <device/virtio_disk.h>
#include <file/file.h>
#include <fs/blkbuf.h>
#include <fs/fs.h>
#include <mm/memlay.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <platform/plic.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <trap/trap.h>
#include <uniks/defs.h>
#include <uniks/kstdio.h>
#include <uniks/kstring.h>
#include <uniks/log.h>

extern void clock_init(), all_interrupt_enable();
char boot_message[] = "uniks boot over!";
volatile static int32_t started = 0;


__noreturn __always_inline void idle_process()
{
	while (1) {
		scheduler();
	}
}
void hart_start_message()
{
	kprintf("[\tuniks]==> hart %d start\n\n", r_mhartid());
}
void kernel_start()
{
	if (!r_mhartid()) {
		memset(sbss, 0, ebss - sbss);
		device_init();
		phymem_init();
		kvminit();
		kvmenable();
		// now, in vaddr space!
		proc_init();
		trap_init();
		plicinit();
		plicinithart();

		// hint: below 4 init functions maybe could let other harts do
		blkbuf_init();
		mount_root();
		sysfile_init();
		virtio_disk_init();

		user_init(1);
		clock_init();
		all_interrupt_enable();
		__sync_synchronize();

		debugf("stext: %p\tetext: %p", stext, etext);
		debugf("sdata: %p\tedata: %p", sdata, edata);
		debugf("sbss: %p\tebss: %p", sbss, ebss);
		debugf("end: %p\n", end);

		kprintf("[\tuniks]==> total memory size: %l MiB, total pages: %d\n",
			PHYMEMSIZE / 1024 / 1024, PHYMEMSIZE / 4096);
		kprintf("[\tuniks]==> %s\n", boot_message);
		hart_start_message();
		started = 1;
	} else {
		while (!started)
			;
		__sync_synchronize();
		kvmenable();
		plicinithart();
		trap_init();
		hart_start_message();
	}
	idle_process();

	panic("will never step here!");
}