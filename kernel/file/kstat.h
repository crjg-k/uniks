#ifndef __KERNEL_FILE_KSTAT_H__
#define __KERNEL_FILE_KSTAT_H__


#include <uniks/defs.h>


typedef uint16_t mode_t;
typedef uint32_t ino_t;
typedef uint32_t dev_t;
typedef uint16_t nlink_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint32_t time_t;

struct stat_t {
	dev_t st_dev;	    /* ID of device containing file */
	ino_t st_ino;	    /* Inode number */
	mode_t st_mode;	    /* File type and mode */
	nlink_t st_nlink;   /* Number of hard links */
	uid_t st_uid;	    /* User ID of owner */
	gid_t st_gid;	    /* Group ID of owner */
	dev_t st_rdev;	    /* Device ID (if special file) */
	size_t st_size;	    /* Total size, in bytes */
	size_t st_blksize;  /* Block size for filesystem I/O */
	uint32_t st_blocks; /* Number of 512B blocks allocated */

	/* Since Linux 2.6, the kernel supports nanosecond
	   precision for the following timestamp fields.
	   For the details before Linux 2.6, see NOTES. */

	time_t st_atime; /* Time of last access */
	time_t st_mtime; /* Time of last modification */
	time_t st_ctime; /* Time of last status change */
};


#endif /* !__KERNEL_FILE_KSTAT_H__ */