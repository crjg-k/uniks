#ifndef __KERNEL_FILE_PIPE_H__
#define __KERNEL_FILE_PIPE_H__


#include <sync/spinlock.h>
#include <uniks/list.h>
#include <uniks/queue.h>


// such a structure is stored in an inode struct in memory inode table
struct pipe_t {
	// corresponding to the first field of `struct ext2_inode_t`
	uint16_t i_mode;

	struct spinlock_t lk;
	struct queue_meta_t pipe;

	uint8_t readopen;    // read fd is still open
	uint8_t writeopen;   // write fd is still open
	struct list_node_t read_wait;
	struct list_node_t write_wait;
};


struct m_inode_t *pipealloc();
void pipeclose(struct pipe_t *pi, int32_t W);
int64_t pipewrite(struct pipe_t *pi, void *addr, size_t n);
int64_t piperead(struct pipe_t *pi, void *addr, size_t n);


#endif /* !__KERNEL_FILE_PIPE_H__ */