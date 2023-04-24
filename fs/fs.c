#include "fs.h"
#include <uniks/param.h>


struct spinlock_t m_inode_table_lock[NINODE];
struct m_inode_t m_inode_table[NINODE];


// this function name is learned from Linux-0.11:fs/super.c
void mount_root()
{
	for (int32_t i = 0; i < NINODE; i++) {
		// initsleeplock(&itable.inode[i].lock, "inode");
	}
}