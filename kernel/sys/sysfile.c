#include <device/blkbuf.h>
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


/**
 * hint: `sys_rename`, `sys_symlink`, `sys_readlink` and any other
 * fd-version syscalls of file system are not implemented temporarily.
 */

#define sys_file_rw_common() \
	struct file_t *f; \
	struct proc_t *p = myproc(); \
	int32_t fd = argufetch(p, 0); \
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1) \
		return -EBADF; \
	size_t cnt = argufetch(p, 2); \
	if (cnt == 0) \
		return 0; \
	char *buf = (char *)argufetch(p, 1); \
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

// `void sync(void);`
void sys_sync()
{
	sync_sb_and_gdt();
	blk_sync_all(0);
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

static struct m_inode_t *create(char *path, uint16_t type, mode_t mode,
				uint16_t major, uint16_t minor)
{
	struct m_inode_t *ip, *dp;
	char name[EXT2_NAME_LEN + 1];

	if ((dp = nameiparent(path, name)) == NULL)
		return NULL;

	ilock(dp);

	if ((ip = dirlookup(dp, name, 0)) != NULL) {
		iunlockput(dp);
		ilock(ip);
		if (type == EXT2_FT_REGFILE and
		    (S_ISREG(ip->d_inode_ctnt.i_mode) or
		     S_ISBLK(ip->d_inode_ctnt.i_mode) or
		     S_ISCHR(ip->d_inode_ctnt.i_mode))) {
			iunlock(ip);
			return ip;
		}
		iunlockput(ip);
		return NULL;
	}

	if ((ip = ialloc(dp->i_dev)) == NULL) {
		iunlockput(dp);
		return NULL;
	}

	ilock(ip);
	ip->d_inode_ctnt.i_size = ip->d_inode_ctnt.i_blocks = 0;
	ip->d_inode_ctnt.i_mode = mode;
	ip->d_inode_ctnt.i_links_count = 1;
	memset(ip->d_inode_ctnt.i_block, 0, sizeof(ip->d_inode_ctnt.i_block));

	// The temporarily unused fields are set to 0 currently.
	ip->d_inode_ctnt.i_flags = ip->d_inode_ctnt.i_uid =
		ip->d_inode_ctnt.i_gid = ip->d_inode_ctnt.i_atime =
			ip->d_inode_ctnt.i_ctime = ip->d_inode_ctnt.i_mtime =
				ip->d_inode_ctnt.i_dtime = 0;
	iupdate(ip, 0);

	if (type == EXT2_FT_DIR) {   // Create "." and ".." entries.
		// No `i_links_count++` for ".": avoid cyclic ref count.
		if (dirlink(ip, ".", ip->i_no) < 0 or
		    dirlink(ip, "..", dp->i_no) < 0)
			goto fail;
		group_descs[ip->i_block_group].bg_used_dirs_count++;
	}

	if (dirlink(dp, name, ip->i_no) < 0)
		goto fail;

	if (type == EXT2_FT_DIR) {
		// now that success is guaranteed:
		dp->d_inode_ctnt.i_links_count++;   // for ".."
		iupdate(dp, 0);
	}

	iunlock(ip);
	iunlockput(dp);

	return ip;

fail:
	// something went wrong. de-allocate `ip`.
	ip->d_inode_ctnt.i_links_count = 0;
	iupdate(ip, 0);
	iunlockput(ip);
	iunlockput(dp);
	return 0;
}

// `int creat(const char *pathname, mode_t mode);`
int64_t sys_creat()
{
	int64_t res;
	struct proc_t *p = myproc();
	char *path = kmalloc(PATH_MAX);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0) {
		res = -EFAULT;
		goto ret;
	}

	mode_t mode = argufetch(p, 1);
	if (create(path, EXT2_FT_REGFILE, (mode | EXT2_S_IFREG), 0, 0) ==
	    NULL) {
		res = -EPERM;
		goto ret;
	}
	res = 0;

ret:
	kfree(path);
	return res;
}

// `int chmod(const char *pathname, mode_t mode);`
int64_t sys_chmod()
{
	int64_t res;
	struct proc_t *p = myproc();
	struct m_inode_t *inode;
	char *path = kmalloc(PATH_MAX);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0) {
		res = -EFAULT;
		goto ret;
	}

	if ((inode = namei(path, 0)) == NULL) {
		res = -ENOENT;
		goto ret;
	}
	mode_t mode = argufetch(p, 1);
	clear_var_bit(inode->d_inode_ctnt.i_mode, 0777);
	set_var_bit(inode->d_inode_ctnt.i_mode, mode);
	iupdate(inode, 0);
	res = 0;

ret:
	kfree(path);
	return res;
}

// `int open(const char *pathname, int flags, mode_t mode);`
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

	uint32_t flags = argufetch(p, 1);
	if (get_var_bit(flags, O_CREAT)) {
		// need the 3rd argument of open() in user mode
		mode_t mode = argufetch(p, 2);
		if ((inode = create(path, EXT2_FT_REGFILE,
				    (mode | EXT2_S_IFREG), 0, 0)) == NULL) {
			fd = -EPERM;
			goto ret1;
		}
	} else if ((inode = namei(path, 0)) == NULL) {
		// if return value<0, release file structure and return errno
		fd = -ENOENT;
		goto ret1;
	}

	// allocate an idle system fcbtable entry
	int32_t fcb_no;
	p->fdtable[fd] = fcb_no = file_alloc();

	struct file_t *f = &fcbtable.files[fcb_no];
	f->f_flags = flags;
	f->f_count++;
	f->f_inode = inode;
	if (get_var_bit(flags, O_APPEND)) {   // append mode
		ilock(inode);
		f->f_pos = inode->d_inode_ctnt.i_size;
		iunlock(inode);
	} else {
		if (get_var_bit(flags, O_TRUNC)) {
			ilock(inode);
			if (itruncate(inode, 0) < 0) {
				fd = -EIO;
				iunlockput(inode);
				goto ret1;
			}
			iunlock(inode);
		}
		f->f_pos = 0;
	}

ret1:
	kfree(path);
ret2:
	return fd;
}

int64_t do_close(int32_t fd)
{
	struct proc_t *p = myproc();
	assert(p->fdtable[fd] != -1);

	file_close(p->fdtable[fd]);
	p->fdtable[fd] = -1;

	return 0;
}

// `int close(int fd);`
int64_t sys_close()
{
	struct proc_t *p = myproc();
	int32_t fd = argufetch(p, 0);
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1)
		return -EBADF;

	return do_close(fd);
}

// Do fd dupping which will be called by `sys_dup2()` and `sys_dup()` both.
static int32_t do_dupfd(uint32_t fd, uint32_t arg)
{
	struct proc_t *p = myproc();

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

	if (newfd >= NFD or newfd < 0)
		return -EBADF;
	if (p->fdtable[newfd] != -1)
		do_close(newfd);

	return do_dupfd(oldfd, newfd);
}

// Copy oldfd to a new fd which is sure that newfd is the min idle fd.
// `int dup(int oldfd);`
int64_t sys_dup()
{
	struct proc_t *p = myproc();
	uint32_t oldfd = argufetch(p, 0);
	if (oldfd >= NFD or oldfd < 0 or p->fdtable[oldfd] == -1)
		return -EBADF;

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
			strncpy(cur, pathname, len);
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

// `int mkdir(const char *pathname, mode_t mode);`
int64_t sys_mkdir()
{
	int64_t res;
	struct proc_t *p = myproc();
	char *path = kmalloc(PATH_MAX);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0) {
		res = -EFAULT;
		goto ret;
	}

	mode_t mode = argufetch(p, 1);
	if (create(path, EXT2_FT_DIR, (mode | EXT2_S_IFDIR), 0, 0) == NULL) {
		res = -EPERM;
		goto ret;
	}
	res = 0;

ret:
	kfree(path);
	return res;
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
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1)
		return -EBADF;

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

int64_t do_stat(struct m_inode_t *ip)
{
	struct proc_t *p = myproc();
	struct stat_t st;

	uintptr_t st_vaddr = argufetch(p, 1);
	if (verify_area(p->mm, st_vaddr, sizeof(st), PTE_R | PTE_W | PTE_U) < 0)
		return -1;

	ilock(ip);
	stati(ip, &st);
	iunlock(ip);

	assert(copyout(p->mm->pagetable, (void *)st_vaddr, (void *)&st,
		       sizeof(st)) != -1);

	return 0;
}

// `int stat(const char *pathname, struct stat *statbuf);`
int64_t sys_stat()
{
	int32_t res = -EFAULT;
	struct proc_t *p = myproc();
	struct m_inode_t *inode;

	char *path = kmalloc(PATH_MAX);
	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0)
		goto ret;

	// if return value<0, release file structure and return errno
	if ((inode = namei(path, 0)) == NULL) {
		res = -ENOENT;
		goto ret;
	}

	if (do_stat(inode) < 0)
		goto ret;
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
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1)
		return -EBADF;

	struct file_t *f = &fcbtable.files[p->fdtable[fd]];
	return do_stat(f->f_inode);
}

// `off_t lseek(int fd, off_t offset, int whence);`
int64_t sys_lseek()
{
#define SEEK_SET 0 /* Seek from beginning of file.  */
#define SEEK_CUR 1 /* Seek from current position.  */
#define SEEK_END 2 /* Seek from end of file.  */

	struct proc_t *p = myproc();
	int32_t fd = argufetch(p, 0);
	if (fd >= NFD or fd < 0 or p->fdtable[fd] == -1)
		return -EBADF;

	struct file_t *f = &fcbtable.files[p->fdtable[fd]];

	size_t whence = argufetch(p, 2);
	if (whence >= SEEK_SET or whence <= SEEK_END) {
		int64_t off = argufetch(p, 1);
		switch (whence) {
		case SEEK_SET:
			f->f_pos = off;
			break;
		case SEEK_CUR:
			f->f_pos += off;
			break;
		case SEEK_END:
			f->f_pos = f->f_inode->d_inode_ctnt.i_size + off;
		}
	} else
		return -EINVAL;

	return 0;
}

// `int truncate(const char *path, off_t length);`
int64_t sys_truncate()
{
	int64_t res;
	struct proc_t *p = myproc();
	struct m_inode_t *inode;
	char *path = kmalloc(PATH_MAX);

	uintptr_t uaddr = argufetch(p, 0);
	if (argstrfetch(uaddr, path, PATH_MAX) < 0) {
		res = -EFAULT;
		goto ret;
	}
	if ((inode = namei(path, 0)) == NULL) {
		res = -ENOENT;
		goto ret;
	}

	ilock(inode);
	if (!S_IWUSR(inode->d_inode_ctnt.i_mode)) {
		res = -EACCES;
		goto ret;
	}

	int64_t length = argufetch(p, 1);
	if (itruncate(inode, length) < 0)
		res = -EIO;
	else
		res = 0;

ret:
	iunlockput(inode);
	kfree(path);
	return res;
}

// `int link(const char *oldpath, const char *newpath);`
int64_t sys_link()
{
	int64_t res;
	struct proc_t *p = myproc();
	struct m_inode_t *ip, *dp;
	char *old_path = kmalloc(PATH_MAX), *new_path = kmalloc(PATH_MAX),
	     name[EXT2_NAME_LEN + 1];

	uintptr_t old_uaddr = argufetch(p, 0), new_uaddr = argufetch(p, 1);
	if (argstrfetch(old_uaddr, old_path, PATH_MAX) < 0 or
	    argstrfetch(new_uaddr, new_path, PATH_MAX) < 0) {
		res = -EFAULT;
		goto ret;
	}

	if ((ip = namei(old_path, 0)) == NULL) {
		res = -ENOENT;
		goto ret;
	}

	ilock(ip);
	if (S_ISDIR(ip->d_inode_ctnt.i_mode)) {
		// `sys_link` DOES NOT allow to link a directory
		iunlockput(ip);
		res = -EPERM;
		goto ret;
	}
	ip->d_inode_ctnt.i_links_count++;
	iupdate(ip, 0);
	iunlock(ip);

	if ((dp = nameiparent(new_path, name)) == NULL)
		goto fail;
	ilock(dp);
	if (dirlink(dp, name, ip->i_no) < 0) {
		iunlockput(dp);
		goto fail;
	}
	iunlockput(dp);
	iput(ip);
	res = 0;
	goto ret;

fail:
	ilock(ip);
	ip->d_inode_ctnt.i_links_count--;
	iupdate(ip, 0);
	iunlockput(ip);
ret:
	kfree(old_path), kfree(new_path);
	return res;
}

// Is the directory `dp` empty except for "." and ".." ?
static int64_t isdirempty(struct m_inode_t *dp)
{
	uint64_t off = 12;   // size of entry "."
	char tmp_de[EXT2_DIRENTRY_MAXSIZE + 1];
	struct ext2_dir_entry_t *de = (struct ext2_dir_entry_t *)tmp_de;
	readi(dp, 0, tmp_de, off, EXT2_DIRENTRY_MAXSIZE);
	assert(strncmp(de->name, "..", 2) == 0);

	return (off + de->rec_len == BLKSIZE);
}

// `int unlink(const char *pathname);`
int64_t sys_unlink()
{
	return 0;
}

// `int rmdir(const char *pathname);`
int64_t sys_rmdir()
{
	return 0;
}
