#include "file.h"
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/errno.h>


// system open files table
struct fcbtable_t fcbtable;


void sys_ftable_init()
{
	initlock(&fcbtable.lock, "fcbtable");
	INIT_LIST_HEAD(&fcbtable.wait_list);
	for (int32_t i = 0; i < NFILE; i++)
		fcbtable.files[i].f_count = 0;

	queue_init(&fcbtable.qm, NFILE, fcbtable.idle_fcb_queue_array);
	for (int32_t i = 0; i < NFILE; i++)
		queue_push_int32type(&fcbtable.qm, i);
}

// allocate an idle file structure entry in fcb table
struct file_t *file_alloc()
{
	acquire(&fcbtable.lock);
	while (queue_empty(&fcbtable.qm)) {
		proc_block(&fcbtable.wait_list, &fcbtable.lock);
	}
	int32_t idlefcb = *(int32_t *)queue_front_int32type(&fcbtable.qm);
	queue_pop(&fcbtable.qm);
	assert(fcbtable.files[idlefcb].f_count == 0);
	fcbtable.files[idlefcb].f_count++;
	release(&fcbtable.lock);
	return &fcbtable.files[idlefcb];
}

// Increment ref count for file f.
void file_dup(struct file_t *f)
{
	acquire(&fcbtable.lock);
	assert(f->f_count >= 1);
	f->f_count++;
	release(&fcbtable.lock);
}

// free a file structure entry pointed by pointer f
static void file_free(struct file_t *f)
{
	queue_push_int32type(&fcbtable.qm, f - fcbtable.files);
	proc_unblock_all(&fcbtable.wait_list);
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void file_close(struct file_t *f)
{
	acquire(&fcbtable.lock);
	if (--f->f_count > 0)
		goto over;

	assert(f->f_count == 0);
	file_free(f);
over:
	release(&fcbtable.lock);
}

// Read from file f.
// addr is a user virtual address.
int32_t file_read(struct file_t *f, uint64_t addr, int32_t cnt)
{
	int32_t res = -1;
	if (f->f_flags)
		goto ret;

	verify_area(myproc()->mm, addr, cnt, PTE_R | PTE_W | PTE_U);
	// struct m_inode_info_t *inode = f->f_inode;
	// if PIPE

	// else if DEVICE

	// else if ordinary disk file, both directory or common file are okay
ret:
	return res;
}

// Write to file f.
// addr is a user virtual address.
int32_t file_write(struct file_t *f, uint64_t addr, int32_t cnt)
{
	int32_t res = -1;
	if (f->f_flags)
		goto ret;
	// struct m_inode_info_t *inode = f->f_inode;
ret:
	return res;
}

// Init fs
void fsinit(dev_t dev)
{
	ext2fs_init(dev);
}