#ifndef __KERNEL_PROCESS_FILE_H__
#define __KERNEL_PROCESS_FILE_H__


#include <defs.h>

#define NDIRECT 12


// todo: maybe some fields of this structure could be transformed into union
struct file {
	enum {
		FD_NONE,
		FD_PIPE,
		FD_INODE,
		FD_DEVICE
	} type;
	int32_t refnum;
	int8_t re;
	int8_t we;
	struct pipe *pipe;   // FD_PIPE
	struct inode *ip;    // FD_INODE and FD_DEVICE
	uint32_t off;	     // FD_INODE
	int16_t major;	     // FD_DEVICE
};

// note: the content of inode is different due to locating at secondary storage
// note: or main memory

// in-memory copy of an inode
struct inode {
	uint64_t valid;	  // inode has been read from disk?
	uint64_t ref;	  // reference count
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

void fileinit();


#endif /* !__KERNEL_PROCESS_FILE_H__ */