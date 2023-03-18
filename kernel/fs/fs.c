#include "fs.h"
#include <param.h>
#include <process/file.h>

struct {
	struct spinlock lock;
	struct inode inode[NINODE];
} inode_table;

void inode_init()
{
	initlock(&inode_table.lock, "inode_table");
	for (uint32_t i = 0; i < NINODE; i++) {
		// initsleeplock(&itable.inode[i].lock, "inode");
	}
}