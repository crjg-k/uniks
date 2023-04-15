#include "fs.h"
#include <process/file.h>
#include <uniks/param.h>

struct {
	struct spinlock_t lock;
	struct inode_t inode[NINODE];
} inode_table;

void inode_init()
{
	initlock(&inode_table.lock, "inode_table");
	for (uint32_t i = 0; i < NINODE; i++) {
		// initsleeplock(&itable.inode[i].lock, "inode");
	}
}