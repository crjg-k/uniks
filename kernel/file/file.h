#ifndef __KERNEL_FILE_FILE_H__
#define __KERNEL_FILE_FILE_H__


#include <fs/ext2fs.h>
#include <sync/mutex.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>
#include <uniks/queue.h>


struct file_t {
	uint32_t f_count;
	uint32_t f_flags;   // access mode(R/W bit)

	uint64_t f_pos;	  // offset for an FD_INODE file
	struct m_inode_t *f_inode;
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
int32_t file_alloc();
int32_t file_dup(int32_t fcb_no);
void fcbno_free(int32_t fcb_no);
void file_close(int32_t fcb_no);
int64_t file_read(struct file_t *f, void *addr, size_t cnt);
int64_t file_write(struct file_t *f, void *addr, size_t cnt);


#endif /* !__KERNEL_FILE_FILE_H__ */