#ifndef __KERNEL_FILE_FILE_H__
#define __KERNEL_FILE_FILE_H__


#include <fs/ext2fs.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>
#include <uniks/queue.h>


struct file_t {
	uint16_t f_count;
	int16_t f_mode;			  // access mode(R/W bit)
	int32_t f_flags;		  // control flag
	struct m_inode_info_t *f_inode;	  // FD_INODE and FD_DEVICE
	uint64_t f_pos;			  // offset for an FD_INODE file
};

struct fcbtable_t {
	struct spinlock_t lock;
	struct file_t files[NFILE];
	struct queue_meta_t qm;
	int32_t idle_fcb_queue_array[NFILE];
	struct list_node_t wait_list;
};

extern struct fcbtable_t fcbtable;


void sys_ftable_init();
struct file_t *file_alloc();
void file_close(struct file_t *f);
int32_t file_read(struct file_t *f, uint64_t addr, int32_t cnt);
int32_t file_write(struct file_t *f, uint64_t addr, int32_t cnt);


void fsinit(dev_t dev);


#endif /* !__KERNEL_FILE_FILE_H__ */