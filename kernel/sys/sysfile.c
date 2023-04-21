#include <device/device.h>
#include <device/tty.h>
#include <process/file.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/errno.h>

char dst[32];

extern int64_t copyin(pagetable_t pagetable, void *dst, void *srcva,
		      uint64_t len);
extern int32_t copyout(pagetable_t pagetable, void *dstva, void *src,
		       uint64_t len);


#define sys_file_rw_common() \
	struct file_t *file; \
	struct proc_t *p = myproc(); \
	struct inode_t *inode; \
	int32_t fd = argufetch(p, 0), count = argufetch(p, 2); \
	if (fd >= NFD or count < 0 or p->files[fd] == 0) \
		return -EINVAL; \
	if (count == 0) \
		return 0; \
	file = &fcbtable[p->files[fd]]; \
	inode = file->inode; \
	++inode;

uint64_t sys_read()
{
	struct proc_t *p = myproc();
	char *buf = (char *)argufetch(p, 1), kernelch;
	size_t cnt = argufetch(p, 2);
	devices[current_tty].read(devices[current_tty].ptr, &kernelch, cnt);
	copyout(p->pagetable, buf, &kernelch, cnt);

	// sys_file_rw_common();
	// if CHRDEV

	// else if BLKDEV

	// else if ordinary disk file, both directory or common file are okay

	return 1;
}

uint64_t sys_write()
{
	struct proc_t *p = myproc();
	char *buf = (char *)argufetch(p, 1), kernelch;
	size_t cnt = argufetch(p, 2);
	copyin(p->pagetable, &kernelch, buf, cnt);
	devices[current_tty].write(devices[current_tty].ptr, &kernelch, cnt);

	// sys_file_rw_common();
	// if CHRDEV

	// else if BLKDEV

	// else if ordinary disk file, only permit non-directory write operation

	return 1;
}
