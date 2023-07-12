#ifndef __FS_FS_H__
#define __FS_FS_H__

#include <sync/spinlock.h>
#include <uniks/defs.h>


#define ROOTINUM    (1)	     // root inode-number
#define BLKSIZE	    (4096)   // block size
#define FILENAMELEN (255)


// in-memory copy of an inode
struct m_inode_t {
	uint16_t i_mode;    // file type and attribute(rwx bit)
	uint16_t i_uid;	    // user id(owner)
	uint64_t i_size;    // file size(bytes count)
	uint64_t i_mtime;   // modify timestamp
	uint8_t i_nlinks;   // hard links count
};

extern struct spinlock_t m_inode_table_lock[];
extern struct m_inode_t m_inode_table[];


void mount_root();


#endif /* !__KERNEL_FS_FS_H__ */