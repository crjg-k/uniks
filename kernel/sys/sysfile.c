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
#include <uniks/param.h>


#define sys_file_rw_common() \
	struct file_t *f; \
	struct proc_t *p = myproc(); \
	int32_t fd = argufetch(p, 0), cnt = argufetch(p, 2); \
	char *buf = (char *)argufetch(p, 1); \
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == 0) \
		return -EBADF; \
	if (cnt < 0) \
		return -EINVAL; \
	if (cnt == 0) \
		return 0; \
	f = &fcbtable.files[p->fdtable[fd]];


int64_t sys_read()
{
	sys_file_rw_common();
	return file_read(f, buf, cnt);
}

int64_t sys_write()
{
	sys_file_rw_common();
	return file_write(f, buf, cnt);
}

int64_t sys_open()
{
	int32_t fd;
	struct m_inode_t *inode;
	struct proc_t *p = myproc();
	char path[MAX_PATH_LEN];

	uintptr_t addr = argufetch(p, 0);
	if (argstrfetch(addr, path, MAX_PATH_LEN) < 0)
		return -EINVAL;
	int32_t flag = argufetch(p, 1);

	// search an idle fdtable entry of current process
	for (fd = 0; fd < NFD; fd++)
		if (!p->fdtable[fd])
			break;
	if (fd >= NFD)
		return -EINVAL;
	p->fdtable[fd] = 1;

	// allocate an idle system fcbtable entry
	int32_t fcb_no = file_alloc();
	p->fdtable[fd] = fcb_no;

	// if return value<0, release file structure and return errno
	if ((inode = namei(path)) == NULL) {
		file_free(fcb_no);
		p->fdtable[fd] = 0;
		return -ENOENT;
	}

	struct file_t *f = &fcbtable.files[fcb_no];
	assert(f->f_count == 0);
	f->f_flags = flag;
	f->f_count++;
	f->f_inode = inode;
	f->f_pos = 0;

	return fd;
}

int64_t do_close(int32_t fd)
{
	struct proc_t *p = myproc();

	if (fd >= NFD or fd < 0 or p->fdtable[fd] == 0)
		return -EBADF;

	file_close(p->fdtable[fd]);
	p->fdtable[fd] = 0;

	return 0;
}

int64_t sys_close()
{
	int32_t fd = argufetch(myproc(), 0);
	return do_close(fd);
}

// do dup fd, called by sys_dup2() and sys_dup() both
static int32_t do_dupfd(uint32_t fd, uint32_t arg)
{
	struct proc_t *p = myproc();
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == 0)
		return -EBADF;

	if (arg >= NFD)
		return -EINVAL;

	/**
	 * @brief find the 1st fd that >= arg but never be used in current
	 * process fdtable
	 */
	while (arg < NFD)
		if (p->fdtable[arg])
			arg++;
		else
			break;

	if (arg >= NFD)
		return -EMFILE;

	file_dup((p->fdtable[arg] = p->fdtable[fd]));
	return arg;
}

/**
 * @brief copy oldfd to a new fd but which is specificed by newfd. Moreover, if
 * newfd has opend, close fisrt
 * @return uint64_t
 */
int64_t sys_dup2()
{
	struct proc_t *p = myproc();
	uint32_t oldfd = argufetch(p, 0), newfd = argufetch(p, 1);
	do_close(newfd);
	return do_dupfd(oldfd, newfd);
}

// copy oldfd to a new fd which is sure that newfd is the min idle fd
int64_t sys_dup()
{
	uint32_t oldfd = argufetch(myproc(), 0);
	return do_dupfd(oldfd, 0);
}