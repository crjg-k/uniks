#include "file.h"
#include "kfcntl.h"
#include <device/device.h>
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/errno.h>


// system open files table
struct fcbtable_t fcbtable;


void sys_ftable_init()
{
	initlock(&fcbtable.lock, "fcbtable");
	INIT_LIST_HEAD(&fcbtable.wait_list);
	for (int32_t i = 1; i < NFILE; i++) {
		mutex_init(&fcbtable.files[i].f_mtx, "fcblock");
		fcbtable.files[i].f_count = 0;
	}

	queue_init(&fcbtable.qm, NFILE, fcbtable.idle_fcb_queue_array);
	for (int32_t i = 1; i < NFILE; i++)
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
void file_dup(int32_t fcb_no)
{
	struct file_t *f = &fcbtable.files[fcb_no];
	mutex_acquire(&f->f_mtx);
	assert(f->f_count >= 1);
	f->f_count++;
	mutex_release(&f->f_mtx);
}

// Free a file structure entry
void file_free(int32_t fcb_no)
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
	mutex_acquire(&f->f_mtx);
	if (--f->f_count == 0) {
		if (S_ISCHR(f->f_inode->d_inode_ctnt.i_mode) or
		    S_ISREG(f->f_inode->d_inode_ctnt.i_mode) or
		    S_ISDIR(f->f_inode->d_inode_ctnt.i_mode)) {
			iput(f->f_inode);
		}
		mutex_release(&f->f_mtx);
		file_free(fcb_no);
	} else
		mutex_release(&f->f_mtx);
}

// Read from file f. Addr is a user virtual address.
int64_t file_read(struct file_t *f, void *addr, int32_t cnt)
{
	int64_t res = -EINVAL;
	if (!READABLE(f->f_flags))
		goto ret2;

	assert(verify_area(myproc()->mm, (uintptr_t)addr, cnt,
			   PTE_R | PTE_W | PTE_U) != -1);
	struct m_inode_t *inode = f->f_inode;
	mutex_acquire(&f->f_mtx);

	// if PIPE
	if (S_ISFIFO(inode->d_inode_ctnt.i_mode)) {
		goto ret1;
	}

	ilock(inode);
	// else if character DEVICE
	if (S_ISCHR(inode->d_inode_ctnt.i_mode)) {
		res = device_read(inode->d_inode_ctnt.i_block[0], 1, addr, cnt);
	}
	// else if ordinary file or directory
	else if (S_ISREG(inode->d_inode_ctnt.i_mode) or
		 S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		res = readi(inode, 1, addr, f->f_pos, cnt);
		f->f_pos += res;
	}

	iunlock(inode);
ret1:
	mutex_release(&f->f_mtx);
ret2:
	return res;
}

// Write to file f. Addr is a user virtual address.
int64_t file_write(struct file_t *f, void *addr, int32_t cnt)
{
	int64_t res = -EINVAL;
	if (!WRITEABLE(f->f_flags))
		goto ret2;

	struct m_inode_t *inode = f->f_inode;
	mutex_acquire(&f->f_mtx);

	// if PIPE
	if (S_ISFIFO(inode->d_inode_ctnt.i_mode)) {
		goto ret1;
	}

	ilock(inode);
	// else if character DEVICE
	if (S_ISCHR(inode->d_inode_ctnt.i_mode)) {
		res = device_write(inode->d_inode_ctnt.i_block[0], 1, addr,
				   cnt);
	}
	// else if ordinary file or directory
	else if (S_ISREG(inode->d_inode_ctnt.i_mode) or
		 S_ISDIR(inode->d_inode_ctnt.i_mode)) {
		BUG();
	}

	iunlock(inode);
ret1:
	mutex_release(&f->f_mtx);
ret2:
	return res;
}