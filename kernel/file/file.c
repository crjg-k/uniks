#include "file.h"
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/errno.h>
#include <uniks/param.h>


// system open files table
struct file_t fcbtable[NFILE];
struct spinlock_t fcblock[NFILE];


void sysfile_init()
{
	for (int32_t i = 0; i < NFILE; i++) {
		fcbtable[i].f_count = 0;
		initlock(&fcblock[i], "fcbtable");
	}
}

// allocate a file structure entry in fcb table
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
