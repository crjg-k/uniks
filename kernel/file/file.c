#include "file.h"
#include "kfcntl.h"
#include "pipe.h"
#include <device/blk_dev.h>
#include <device/device.h>
#include <fs/ext2fs.h>
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/errno.h>


// hint: DO NOT support file hole now
struct fcbtable_t fcbtable;   // system open files table


void sys_ftable_init()
{
	initlock(&fcbtable.lock, "fcbtable");
	INIT_LIST_HEAD(&fcbtable.wait_list);
	for (int64_t i = 0; i < NFILE; i++)
		fcbtable.files[i].f_count = 0;

	queue_init(&fcbtable.qm, NFILE, fcbtable.idle_fcb_queue_array);
	for (int64_t i = 0; i < NFILE; i++)
		queue_push_int32type(&fcbtable.qm, i);
}

// Allocate an idle file structure entry in fcb table
int32_t file_alloc()
{
	acquire(&fcbtable.lock);
	while (queue_empty(&fcbtable.qm)) {
		proc_block(&fcbtable.wait_list, &fcbtable.lock);
	}
	assert(!queue_empty(&fcbtable.qm));

	int32_t idlefcb = *(int32_t *)queue_front_int32type(&fcbtable.qm);
	queue_front_pop(&fcbtable.qm);
	assert(fcbtable.files[idlefcb].f_count == 0);
	release(&fcbtable.lock);
	return idlefcb;
}

// Increment ref count for file fcb_no.
int32_t file_dup(int32_t fcb_no)
{
	if (fcb_no < 0)
		return fcb_no;
	struct file_t *f = &fcbtable.files[fcb_no];
	acquire(&fcbtable.lock);
	assert(f->f_count >= 1);
	f->f_count++;
	release(&fcbtable.lock);
	return fcb_no;
}

// Free a file structure entry
void fcbno_free(int32_t fcb_no)
{
	acquire(&fcbtable.lock);
	queue_push_int32type(&fcbtable.qm, fcb_no);
	proc_unblock_all(&fcbtable.wait_list);
	release(&fcbtable.lock);
}

// Close file fcb_no. (Decrement ref count, close when reaches 0.)
void file_close(int32_t fcb_no)
{
	struct file_t *f = &fcbtable.files[fcb_no];

	acquire(&fcbtable.lock);
	struct m_inode_t *inode = f->f_inode;
	int32_t flag = f->f_flags;
	assert(f->f_count >= 1);
	if (--f->f_count > 0) {
		release(&fcbtable.lock);
		return;
	}
	release(&fcbtable.lock);

	if (S_ISCHR(inode->d_inode_ctnt.i_mode) or
	    S_ISBLK(inode->d_inode_ctnt.i_mode) or
	    S_ISREG(inode->d_inode_ctnt.i_mode) or
	    S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		iput(inode);
	} else if (S_ISFIFO(inode->d_inode_ctnt.i_mode)) {
		mutex_acquire(&inode->i_mtx);
		pipeclose(&inode->pipe_node, WRITEABLE(flag));
		inode->i_count--;
		mutex_release(&inode->i_mtx);
	}

	fcbno_free(fcb_no);
}

// Read from file f. Addr is a user virtual address.
int64_t file_read(struct file_t *f, void *addr, size_t cnt)
{
	int64_t res = -EINVAL;
	if (!READABLE(f->f_flags))
		goto ret;
	if (verify_area(myproc()->mm, (uintptr_t)addr, cnt,
			PTE_R | PTE_W | PTE_U) < 0) {
		res = -EFAULT;
		goto ret;
	}

	struct m_inode_t *inode = f->f_inode;

	// if PIPE
	if (S_ISFIFO(inode->d_inode_ctnt.i_mode)) {
		res = piperead(&inode->pipe_node, addr, cnt);
		goto ret;
	}

	ilock(inode);
	// else if character DEVICE
	if (S_ISCHR(inode->d_inode_ctnt.i_mode)) {
		res = device_read(inode->d_inode_ctnt.i_block[0], 1, addr, cnt);
	}
	// else if block DEVICE
	else if (S_ISBLK(inode->d_inode_ctnt.i_mode)) {
		if ((res = blkdev_read(inode->d_inode_ctnt.i_block[0], addr,
				       f->f_pos, cnt)) > 0)
			f->f_pos += res;
	}
	// else if ordinary file or directory
	else if (S_ISREG(inode->d_inode_ctnt.i_mode) or
		 S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		if ((res = readi(inode, 1, addr, f->f_pos, cnt)) > 0)
			f->f_pos += res;
	}

	iunlock(inode);
ret:
	return res;
}

// Write to file f. Addr is a user virtual address.
int64_t file_write(struct file_t *f, void *addr, size_t cnt)
{
	int64_t res = -EINVAL;
	if (!WRITEABLE(f->f_flags))
		goto ret;

	if (verify_area(myproc()->mm, (uintptr_t)addr, cnt, PTE_R | PTE_U) <
	    0) {
		res = -EFAULT;
		goto ret;
	}

	struct m_inode_t *inode = f->f_inode;

	// if PIPE
	if (S_ISFIFO(inode->d_inode_ctnt.i_mode)) {
		res = pipewrite(&inode->pipe_node, addr, cnt);
		goto ret;
	}

	ilock(inode);
	// else if character DEVICE
	if (S_ISCHR(inode->d_inode_ctnt.i_mode)) {
		res = device_write(inode->d_inode_ctnt.i_block[0], 1, addr,
				   cnt);
	}
	// else if block DEVICE
	else if (S_ISBLK(inode->d_inode_ctnt.i_mode)) {
		if ((res = blkdev_write(inode->d_inode_ctnt.i_block[0], addr,
					f->f_pos, cnt)) > 0)
			f->f_pos += res;
	}
	// else if ordinary file or directory
	else if (S_ISREG(inode->d_inode_ctnt.i_mode) or
		 S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		if ((res = writei(inode, 1, addr, f->f_pos, cnt)) > 0)
			f->f_pos += res;
	}

	iunlock(inode);
ret:
	return res;
}