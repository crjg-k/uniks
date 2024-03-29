#include <device/blkbuf.h>
#include <device/device.h>
#include <device/tty.h>
#include <device/virtio_disk.h>
#include <file/file.h>
#include <file/kfcntl.h>
#include <file/pipe.h>
#include <fs/ext2fs.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/errno.h>
#include <uniks/kassert.h>
#include <uniks/param.h>


#define sys_file_rw_common() \
	struct file_t *f; \
	struct proc_t *p = myproc(); \
	int32_t fd = argufetch(p, 0); \
	int64_t cnt = argufetch(p, 2); \
	char *buf = (char *)argufetch(p, 1); \
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1) \
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

#define sys_file_fd_common(fd) \
	for (fd = 0; fd < NFD; fd++) \
		if (p->fdtable[fd] == -1) \
			break; \
	if (fd >= NFD) { \
		fd = -EINVAL; \
		goto ret; \
	}

int64_t sys_open()
{
	int32_t fd;
	struct m_inode_t *inode;
	struct proc_t *p = myproc();

	char *path = kmalloc(MAX_PATH_LEN);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, MAX_PATH_LEN) < 0) {
		fd = -EFAULT;
		goto ret;
	}

	// if return value<0, release file structure and return errno
	if ((inode = namei(path, 0)) == NULL) {
		fd = -ENOENT;
		goto ret;
	}

	// search an idle fdtable entry of current process
	sys_file_fd_common(fd);

	// allocate an idle system fcbtable entry
	int32_t fcb_no;
	p->fdtable[fd] = fcb_no = file_alloc();

	struct file_t *f = &fcbtable.files[fcb_no];
	f->f_flags = argufetch(p, 1);
	f->f_count++;
	f->f_inode = inode;
	f->f_pos = 0;

ret:
	kfree(path);
	return fd;
}

int64_t do_close(int32_t fd)
{
	struct proc_t *p = myproc();

	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1)
		return -EBADF;

	file_close(p->fdtable[fd]);
	p->fdtable[fd] = -1;

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
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1)
		return -EBADF;

	if (arg >= NFD)
		return -EINVAL;

	/**
	 * @brief find the 1st fd that >= arg but never be used in current
	 * process fdtable
	 */
	while (arg < NFD)
		if (p->fdtable[arg] != -1)
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
 * newfd has opened, close fisrt
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

int64_t sys_pipe()
{
	int32_t fd1, fd2, res = -EFAULT;
	struct proc_t *p = myproc();
	uintptr_t pipefd = argufetch(myproc(), 0);
	if (verify_area(p->mm, pipefd, 2 * sizeof(fd1), PTE_W | PTE_U) < 0) {
		goto ret;
	}

	int32_t fcb_no1, fcb_no2;
	sys_file_fd_common(fd1);
	p->fdtable[fd1] = fcb_no1 = file_alloc();
	sys_file_fd_common(fd2);
	p->fdtable[fd2] = fcb_no2 = file_alloc();

	struct m_inode_t *inode = pipealloc();

	struct file_t *f1 = &fcbtable.files[fcb_no1],
		      *f2 = &fcbtable.files[fcb_no2];
	f1->f_flags = O_RDONLY;
	f1->f_count++;
	f1->f_inode = inode;
	f2->f_flags = O_WRONLY;
	f2->f_count++;
	f2->f_inode = idup(inode);

	assert(copyout(p->mm->pagetable, (void *)pipefd, (char *)&fd1,
		       sizeof(fd1)) != -1);
	assert(copyout(p->mm->pagetable, (void *)pipefd + sizeof(fd1),
		       (char *)&fd2, sizeof(fd2)) != -1);
	res = 0;
ret:
	return res;
}