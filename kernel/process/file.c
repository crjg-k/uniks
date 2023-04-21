#include "file.h"
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/param.h>


// system open files table
struct file_t fcbtable[NFILE];
struct spinlock_t fcblock[NFILE];

void file_init()
{
	for (struct spinlock_t *flk = fcblock; flk < &fcblock[NFILE]; flk++)
		initlock(flk, "fcbtable");
}

// allocate a file structure
struct file *file_alloc()
{
	// struct file *f;

	// acquire(&ftable.lock);
	// for (f = ftable.file; f < ftable.file + NFILE; f++) {
	// 	if (f->ref == 0) {
	// 		f->ref = 1;
	// 		release(&ftable.lock);
	// 		return f;
	// 	}
	// }
	// release(&ftable.lock);
	return NULL;
}

// increment ref count for file f
struct file *file_dup(struct file *f)
{
	// acquire(&ftable.lock);
	// if (f->ref < 1)
	// 	panic("filedup");
	// f->ref++;
	// release(&ftable.lock);
	return f;
}


// read from file f and vaddr is a user virtual address.
int32_t file_read(uint32_t fd, char *buf, int32_t count) {
	return 1;
}

// write to file f and vaddr is a user virtual address.
int32_t file_write(uint32_t fd, char *buf, int32_t count) {
	return 1;
}