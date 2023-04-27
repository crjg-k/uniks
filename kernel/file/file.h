#ifndef __KERNEL_FILE_FILE_H__
#define __KERNEL_FILE_FILE_H__

#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/param.h>
#include <uniks/queue.h>


struct file_t {
	uint16_t f_count;
	int16_t f_mode;	  // access mode
	int32_t f_flags;
	struct m_inode_t *f_inode;   // FD_INODE and FD_DEVICE
	int64_t f_pos;		     // offset for an FD_INODE file
};

struct idle_fcb_queue_t {
	struct spinlock_t idle_fcb_lock;
	struct queue_meta_t qm;
	int32_t idle_fcb_queue_array[NFILE];
};

extern struct file_t fcbtable[];
extern struct idle_fcb_queue_t idle_fcb_queue;


void sysfile_init();
struct file_t *file_alloc();
void file_free(struct file_t *f);


#endif /* !__KERNEL_FILE_FILE_H__ */