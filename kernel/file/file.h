#ifndef __KERNEL_FILE_FILE_H__
#define __KERNEL_FILE_FILE_H__

#include <sync/spinlock.h>
#include <uniks/defs.h>


struct file_t {
	uint16_t f_count;
	int16_t f_mode;	  // access mode
	int32_t f_flags;
	struct m_inode_t *f_inode;   // FD_INODE and FD_DEVICE
	int64_t f_pos;		     // offset for a FD_INODE file
};

extern struct file_t fcbtable[];
extern struct spinlock_t fcblock[];


void sysfile_init();
struct file *file_alloc();


#endif /* !__KERNEL_FILE_FILE_H__ */