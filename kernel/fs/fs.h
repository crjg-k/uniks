#ifndef __KERNEL_FS_FS_H__
#define __KERNEL_FS_FS_H__


#define FSMAGIC 0x114514

#define ROOTINUM 1	// root i-number
#define BLKSIZE	 1024	// block size


#include <sync/spinlock.h>
#include <uniks/defs.h>


// note: the content of inode is different due to locating at secondary storage
// note: or main memory


void inode_init();


#endif /* !__KERNEL_FS_FS_H__ */