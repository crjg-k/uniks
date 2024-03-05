/**
 * @file kernelinit.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-01-29 15:55:51
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */

#include <device/blkbuf.h>
#include <device/clock.h>
#include <device/device.h>
#include <device/virtio_disk.h>
#include <file/file.h>
#include <fs/ext2fs.h>
#include <mm/memlay.h>
#include <platform/plic.h>
#include <platform/sbi.h>
#include <process/proc.h>
#include <trap/trap.h>
#include <uniks/banner.h>
#include <uniks/defs.h>
#include <uniks/kstdio.h>
#include <uniks/kstring.h>


volatile static int32_t started = 0, master_booting = 1;
int32_t boothartid;
// entry.S needs one stack per HART.
__aligned(PGSIZE) char hart_stacks[KSTACKSIZE * MAXNUM_HARTID];

__noreturn __always_inline void idle_process()
{
	struct cpu_t *c = mycpu();
	c->proc = pcbtable[0];
	interrupt_on();
	while (1) {
		scheduler(c);
	}
}
__always_inline void display_banner()
{
	kprintf(UNIKS_BANNER);
}
__always_inline void hart_booted_message()
{
	kprintf("%shart %d start\n", UNIKS_MSG, cpuid());
}
void kernel_start(int32_t hartid)
{
	if (master_booting) {
		// this section will not be executed concurrently
		memset(sbss, 0, ebss - sbss);
		boothartid = hartid;
		master_booting = 0;
		__sync_synchronize();
		// Don't worry, this macro is defined in makefile!
		for (int32_t i = 0; i < MAXNUM_HARTID; i++) {
			if (i != boothartid)
				sbi_hart_start(i, (uintptr_t)kernel_entry, 0);
		}
	}
	cpus[hartid].hartid = hartid;
	cpus[hartid].repeat = 0;
	write_csr(sscratch, &cpus[hartid]);

	if (cpuid() == boothartid) {
		kprintfinit();
		device_init();
		phymem_init();
		kvminit();

		proc_init();
		trap_init();
		plicinit();

		blk_init();
		inode_table_init();
		sys_ftable_init();
		virtio_disk_init();

		user_init(1);
		display_banner();
		hart_booted_message();
		started = 1;
		__sync_synchronize();
	} else {
		while (!started)
			;
		__sync_synchronize();
		hart_booted_message();
	}
	kvmenablehart();
	// now, in vaddr space!
	plicinithart();
	trap_inithart();
	clock_init();
	all_interrupt_enable();

	idle_process();

	BUG();
}