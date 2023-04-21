#ifndef __KERNEL_PROCESS_FILE_H__
#define __KERNEL_PROCESS_FILE_H__

#include <sync/spinlock.h>
#include <uniks/defs.h>

#define NDIRECT 12


// todo: maybe some fields of this structure could be transformed into union
struct file_t {
	uint16_t refcnt;
	int16_t mode;	// access mode
	int32_t flag;
	struct inode_t *inode;	 // FD_INODE and FD_DEVICE
	int64_t offset_pos;	 // offset for a FD_INODE file
};

extern struct file_t fcbtable[];
extern struct spinlock_t fcblock[];


// note: the content of inode is different due to locating at secondary storage
// note: or main memory

// in-memory copy of an inode
struct inode_t {
	uint64_t valid;	   // inode has been read from disk?
	uint64_t refcnt;   // reference count
	uint32_t device;
	uint32_t inodenum;
	// struct sleeplock lock;	 // protects everything below here

	uint16_t type;
	uint16_t major;
	uint16_t minor;
	uint16_t nlink;
	uint32_t size;
	uint32_t addr[NDIRECT + 1];
};

void file_init();
struct file *file_alloc();
struct file *file_dup(struct file *f);
int32_t file_read(uint32_t fd, char *buf, int32_t n);
int32_t file_write(uint32_t fd, char *buf, int32_t n);


#endif /* !__KERNEL_PROCESS_FILE_H__ */