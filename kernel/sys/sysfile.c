#include <device/blkbuf.h>
#include <device/device.h>
#include <device/tty.h>
#include <device/virtio_disk.h>
#include <file/file.h>
#include <fs/ext2fs.h>
#include <mm/phys.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/errno.h>
#include <uniks/kassert.h>


#define sys_file_rw_common() \
	struct file_t *f; \
	struct proc_t *p = myproc(); \
	struct m_inode_t *inode; \
	int32_t fd = argufetch(p, 0), cnt = argufetch(p, 2); \
	char *buf = (char *)argufetch(p, 1); \
	if (fd >= NFD or cnt < 0 or p->fdtable[fd] == 0) \
		return -EINVAL; \
	if (cnt == 0) \
		return 0; \
	f = &fcbtable.files[p->fdtable[fd]];


uint64_t sys_read()
{
	// sys_file_rw_common();
	// return file_read(f, buf, cnt);
	uint64_t res;
	struct proc_t *p = myproc();
	size_t cnt = argufetch(p, 2);
	char *buf = (char *)argufetch(p, 1);
	res = cnt;
	struct blkbuf_t *bb = blk_read(VIRTIO_IRQ, 0);
	// res = devices[current_tty].read(devices[current_tty].ptr, tmp_page,
	// 				cnt);
	verify_area(p->mm, (uintptr_t)buf, cnt, PTE_W | PTE_U);
	copyout(p->mm->pagetable, buf, bb->b_data, res);
	blk_release(bb);

	return PGSIZE;
}

uint64_t sys_write()
{
	// sys_file_rw_common();
	// return file_write(f, buf, cnt);
	uint64_t res;
	struct proc_t *p = myproc();
	char *buf = (char *)argufetch(p, 1), *tmp_page = (char *)pages_alloc(1);
	assert(tmp_page != NULL);
	size_t cnt = argufetch(p, 2);
	int32_t fd = argufetch(p, 0);
	if (fd == 3) {
		struct blkbuf_t *bb = blk_read(VIRTIO_IRQ, 0);
		copyin(p->mm->pagetable, bb->b_data, buf, cnt);
		blk_write_over(bb);
		blk_sync_all();
		res = PGSIZE;
	} else {
		copyin(p->mm->pagetable, tmp_page, buf, cnt);
		res = devices[current_tty].write(devices[current_tty].ptr,
						 tmp_page, cnt);
	}
	pages_free(tmp_page);

	return res;
}

uint64_t sys_open()
{
	int32_t i, fd;
	// struct m_inode_info_t *inode;
	struct file_t *f;
	struct proc_t *p = myproc();

	// char *filename = (char *)argufetch(p, 0);
	// int32_t flag = argufetch(p, 1), mode = argufetch(p, 2);

	// search a idle fdtable entry of current process
	for (fd = 0; fd < NFD; fd++)
		if (!p->fdtable[fd])
			break;
	if (fd >= NFD)
		return -EINVAL;
	// search a idle system fcbtable entry
	for (i = 0; i < NFILE; i++)
		if (!((f = &fcbtable.files[i])->f_count))
			break;
	if (i >= NFILE)
		return -EINVAL;
	// setup link and update reference count
	p->fdtable[fd] = i;
	f->f_count++;

	// if return value<0, release file structure and return errno
	// if ((i = open_namei(filename, flag, mode, &inode)) < 0) {
	// 	p->fdtable[fd] = 0;
	// 	f->f_count = 0;
	// 	return i;
	// }

	// f->f_mode = inode->i_mode;
	// f->f_flags = flag;
	f->f_count = 1;
	// f->f_inode = inode;
	f->f_pos = 0;
	return fd;
}

uint64_t do_close(uint32_t fd)
{
	struct file_t *f;

	struct proc_t *p = myproc();
	uint32_t fcb_idx;

	acquire(&pcblock[p->pid]);
	if (fd >= NFD or p->fdtable[fd] == 0) {
		release(&pcblock[p->pid]);
		return -EINVAL;
	}
	fcb_idx = p->fdtable[fd];
	p->fdtable[fd] = 0;
	release(&pcblock[p->pid]);

	f = &fcbtable.files[fcb_idx];

	assert(f->f_count != 0);
	if (--f->f_count)
		goto over;

	// release an inode
	// iput(f->f_inode);
over:
	return 0;
}

uint64_t sys_close()
{
	uint32_t fd = argufetch(myproc(), 0);
	return do_close(fd);
}

// do dup fd, called by sys_dup2() and sys_dup() both
static int32_t do_dupfd(uint32_t fd, uint32_t arg)
{
	struct proc_t *p = myproc();
	acquire(&pcblock[p->pid]);
	if (fd >= NFD or p->fdtable[fd] == 0) {
		return -EBADF;
		release(&pcblock[p->pid]);
	}

	if (arg >= NFD)
		return -EINVAL;

	/**
	 * @brief find the 1st fd that >= arg but never be used in current
	 * process fdtable
	 */
	acquire(&pcblock[p->pid]);
	while (arg < NFD)
		if (p->fdtable[arg])
			arg++;
		else
			break;

	if (arg >= NFD) {
		release(&pcblock[p->pid]);
		return -EMFILE;
	}
	p->fdtable[arg] = p->fdtable[fd];
	fcbtable.files[p->fdtable[arg]].f_count++;
	release(&pcblock[p->pid]);
	return arg;
}

/**
 * @brief copy oldfd to a new fd but which is specificed by newfd. Moreover, if
 * newfd has opend, close fisrt
 * @return uint64_t
 */
uint64_t sys_dup2()
{
	struct proc_t *p = myproc();
	uint32_t oldfd = argufetch(p, 0), newfd = argufetch(p, 1);
	do_close(newfd);
	return do_dupfd(oldfd, newfd);
}

// copy oldfd to a new fd which is sure that newfd is the min idle fd
uint64_t sys_dup()
{
	struct proc_t *p = myproc();
	uint32_t oldfd = argufetch(p, 0);
	return do_dupfd(oldfd, 0);
}