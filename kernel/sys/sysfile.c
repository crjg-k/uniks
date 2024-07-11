#include <file/file.h>
#include <file/kfcntl.h>
#include <file/kstat.h>
#include <file/pipe.h>
#include <fs/ext2fs.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <process/proc.h>
#include <sys/ksyscall.h>
#include <uniks/defs.h>
#include <uniks/errno.h>
#include <uniks/kassert.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>
#include <uniks/param.h>


#define sys_file_rw_common() \
	struct file_t *f; \
	struct proc_t *p = myproc(); \
	int32_t fd = argufetch(p, 0); \
	size_t cnt = argufetch(p, 2); \
	char *buf = (char *)argufetch(p, 1); \
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1) \
		return -EBADF; \
	if (cnt == 0) \
		return 0; \
	f = &fcbtable.files[p->fdtable[fd]];


// `size_t read(int fd, void *buf, size_t count);`
int64_t sys_read()
{
	sys_file_rw_common();
	return file_read(f, buf, cnt);
}

// `size_t write(int fd, const void *buf, size_t count);`
int64_t sys_write()
{
	sys_file_rw_common();
	return file_write(f, buf, cnt);
}

// Search an idle fdtable entry of current process.
#define sys_file_fd_common(fd, ret) \
	for (fd = 0; fd < NFD; fd++) \
		if (p->fdtable[fd] == -1) \
			break; \
	if (fd >= NFD) { \
		fd = -EMFILE; \
		goto ret; \
	}

// `int open(const char *pathname, int flags);`
int64_t sys_open()
{
	int32_t fd;
	struct m_inode_t *inode;
	struct proc_t *p = myproc();

	sys_file_fd_common(fd, ret2);

	char *path = kmalloc(PATH_MAX);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0) {
		fd = -EFAULT;
		goto ret1;
	}

	// if return value<0, release file structure and return errno
	if ((inode = namei(path, 0)) == NULL) {
		fd = -ENOENT;
		goto ret1;
	}

	// allocate an idle system fcbtable entry
	int32_t fcb_no;
	p->fdtable[fd] = fcb_no = file_alloc();

	struct file_t *f = &fcbtable.files[fcb_no];
	f->f_flags = argufetch(p, 1);
	f->f_count++;
	f->f_inode = inode;
	f->f_pos = 0;

ret1:
	kfree(path);
ret2:
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

// `int close(int fd);`
int64_t sys_close()
{
	int32_t fd = argufetch(myproc(), 0);
	return do_close(fd);
}

// Do fd dupping which will be called by `sys_dup2()` and `sys_dup()` both.
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
 * newfd has opened, close fisrt.
 * `int dup2(int oldfd, int newfd);`
 * @return uint64_t
 */
int64_t sys_dup2()
{
	struct proc_t *p = myproc();
	uint32_t oldfd = argufetch(p, 0), newfd = argufetch(p, 1);
	do_close(newfd);
	return do_dupfd(oldfd, newfd);
}

// Copy oldfd to a new fd which is sure that newfd is the min idle fd.
// `int dup(int oldfd);`
int64_t sys_dup()
{
	uint32_t oldfd = argufetch(myproc(), 0);
	return do_dupfd(oldfd, 0);
}

// `int pipe(int pipefd[2]);`
int64_t sys_pipe()
{
	int32_t fd1, fd2, res = -EFAULT;
	int32_t fcb_no1, fcb_no2;

	struct proc_t *p = myproc();

	sys_file_fd_common(fd1, ret2);
	p->fdtable[fd1] = fcb_no1 = file_alloc();
	sys_file_fd_common(fd2, ret1);
	p->fdtable[fd2] = fcb_no2 = file_alloc();

	uintptr_t pipefd = argufetch(myproc(), 0);
	if (verify_area(p->mm, pipefd, 2 * sizeof(fd1), PTE_R | PTE_W | PTE_U) <
	    0) {
		goto ret3;
	}

	struct m_inode_t *inode = pipealloc();
	if (inode == NULL) {
		res = -ENOMEM;
		goto ret3;
	}

	struct file_t *f1 = &fcbtable.files[fcb_no1],
		      *f2 = &fcbtable.files[fcb_no2];
	f1->f_flags = O_RDONLY;
	f1->f_count++;
	f1->f_inode = inode;
	f2->f_flags = O_WRONLY;
	f2->f_count++;
	f2->f_inode = idup(inode);

	assert(copyout(p->mm->pagetable, (void *)pipefd, (void *)&fd1,
		       sizeof(fd1)) != -1);
	assert(copyout(p->mm->pagetable, (void *)pipefd + sizeof(fd1),
		       (void *)&fd2, sizeof(fd2)) != -1);
	res = 0;
	goto ret3;

ret1:
	p->fdtable[fd1] = -1;
	fd1 = fd2;
	fcbno_free(fcb_no1);
ret2:
	res = fd1;
ret3:
	return res;
}

// `char *getcwd(char *buf, size_t size);`
int64_t sys_getcwd()
{
	struct proc_t *p = myproc();

	uintptr_t buf = argufetch(p, 0);
	size_t size = argufetch(p, 1);
	size = MIN(size, PATH_MAX);
	if (verify_area(p->mm, buf, size, PTE_R | PTE_W | PTE_U) < 0)
		return 0;

	assert(copyout(p->mm->pagetable, (void *)buf, p->cwd, size) != -1);
	return buf;
}

void abspath(char *pwd, const char *pathname)
{
	static char tok[] = "/\\";
	char *cur = NULL;
	char *ptr = NULL;
	if (is_separator(pathname[0], tok)) {
		cur = pwd + 1;
		*cur = 0;
		pathname++;
	} else {
		cur = strrsep(pwd, tok) + 1;
		*cur = 0;
	}

	while (pathname[0]) {
		ptr = strsep(pathname, tok);
		if (!ptr) {
			break;
		}

		int32_t len = (ptr - pathname) + 1;
		*ptr = '/';
		if (!memcmp(pathname, "./", 2)) {
			/* code */
		} else if (!memcmp(pathname, "../", 3)) {
			if (cur - 1 != pwd) {
				*(cur - 1) = 0;
				cur = strrsep(pwd, tok) + 1;
				*cur = 0;
			}
		} else {
			strncpy(cur, pathname, len + 1);
			cur += len;
		}
		pathname += len;
	}

	if (!pathname[0])
		return;

	if (!strcmp(pathname, "."))
		return;

	if (strcmp(pathname, "..")) {
		strcpy(cur, pathname);
		cur += strlen(pathname);
		*cur = '/';
		*(cur + 1) = '\0';
		return;
	}
	if (cur - 1 != pwd) {
		*(cur - 1) = 0;
		cur = strrsep(pwd, tok) + 1;
		*cur = 0;
	}
}

// `int chdir(const char *path);`
int64_t sys_chdir()
{
	int32_t res = 0;
	struct m_inode_t *inode;
	struct proc_t *p = myproc();

	char *path = kmalloc(PATH_MAX);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0) {
		res = -EFAULT;
		goto ret2;
	}
	if ((inode = namei(path, 0)) == NULL) {
		res = -ENOENT;
		goto ret2;
	}
	if (inode == p->icwd)
		goto ret2;

	ilock(inode);
	if (!S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		res = -EPERM;
		goto ret1;
	}

	abspath(p->cwd, path);
	iput(p->icwd);
	p->icwd = inode;

ret1:
	iunlock(inode);
ret2:
	kfree(path);
	return res;
}

// `long getdents(int fd, struct dirent *dirp, size_t count);`
int64_t sys_getdents()
{
	int32_t res = 0, entrylen;
	struct proc_t *p = myproc();

	int32_t fd = argufetch(p, 0);
	struct file_t *f = &fcbtable.files[p->fdtable[fd]];
	struct m_inode_t *inode = f->f_inode;
	ilock(inode);
	if (!S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		res = -EBADF;
		goto ret;
	}

	uintptr_t buf = argufetch(p, 1);
	size_t nbytes = argufetch(p, 2);
	char tmp_de[EXT2_DIRENTRY_MAXSIZE + 1];
	struct ext2_dir_entry_t *de = (struct ext2_dir_entry_t *)tmp_de;
	while (res < nbytes) {
		if (!readi(inode, 0, tmp_de, f->f_pos, EXT2_DIRENTRY_MAXSIZE))
			break;
		entrylen = sizeof(struct ext2_dir_entry_t) + de->name_len;
		entrylen = alignaddr_up(entrylen + 1, 4);
		if (verify_area(p->mm, buf, entrylen, PTE_R | PTE_W | PTE_U) <
		    0)
			break;
		de->name[de->name_len] = '\0';
		f->f_pos += de->rec_len;
		de->rec_len = entrylen;
		assert(copyout(p->mm->pagetable, (void *)buf, tmp_de,
			       entrylen) != -1);
		res += entrylen;
		buf += entrylen;
	}

ret:
	iunlock(inode);
	return res;
}

// `int stat(const char *pathname, struct stat *statbuf);`
int64_t sys_stat()
{
	int32_t res = -EFAULT;
	struct proc_t *p = myproc();
	struct m_inode_t *inode;
	struct stat_t st;

	char *path = kmalloc(PATH_MAX);
	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0)
		goto ret;

	uintptr_t st_vaddr = argufetch(p, 1);
	if (verify_area(p->mm, st_vaddr, sizeof(st), PTE_R | PTE_W | PTE_U) < 0)
		goto ret;

	// if return value<0, release file structure and return errno
	if ((inode = namei(path, 0)) == NULL) {
		res = -ENOENT;
		goto ret;
	}

	ilock(inode);
	stati(inode, &st);
	iunlock(inode);

	assert(copyout(p->mm->pagetable, (void *)st_vaddr, (void *)&st,
		       sizeof(st)) != -1);
	res = 0;

ret:
	kfree(path);
	return res;
}

// `int fstat(int fd, struct stat *statbuf);`
int64_t sys_fstat()
{
	struct proc_t *p = myproc();

	int32_t fd = argufetch(p, 0);
	struct file_t *f = &fcbtable.files[p->fdtable[fd]];
	struct m_inode_t *inode = f->f_inode;

	struct stat_t st;
	uintptr_t st_vaddr = argufetch(p, 1);
	if (verify_area(p->mm, st_vaddr, sizeof(st), PTE_R | PTE_W | PTE_U) < 0)
		return -EFAULT;

	ilock(inode);
	stati(inode, &st);
	iunlock(inode);

	assert(copyout(p->mm->pagetable, (void *)st_vaddr, (void *)&st,
		       sizeof(st)) != -1);

	return 0;
}

// `int lstat(const char *pathname, struct stat *statbuf);`
int64_t sys_lstat()
{
	return 0;
}
