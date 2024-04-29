#ifndef __USER_INCLUDE_USTAT_H__
#define __USER_INCLUDE_USTAT_H__


// -- file format --
#define S_IFSOCK	       0xC000 /* socket */
#define S_IFLNK		       0xA000 /* symbolic link */
#define S_IFREG		       0x8000 /* regular file */
#define S_IFBLK		       0x6000 /* block device */
#define S_IFDIR		       0x4000 /* directory */
#define S_IFCHR		       0x2000 /* character device */
#define S_IFIFO		       0x1000 /* fifo */
/* Test macros for file types.	*/
#define __S_ISTYPE(mode, mask) ((mode & 0xf000) == (mask))
#define S_ISSOCK(mode)	       __S_ISTYPE((mode), S_IFSOCK)
#define S_ISLNK(mode)	       __S_ISTYPE((mode), S_IFLNK)
#define S_ISREG(mode)	       __S_ISTYPE((mode), S_IFREG)
#define S_ISBLK(mode)	       __S_ISTYPE((mode), S_IFBLK)
#define S_ISDIR(mode)	       __S_ISTYPE((mode), S_IFDIR)
#define S_ISCHR(mode)	       __S_ISTYPE((mode), S_IFCHR)
#define S_ISFIFO(mode)	       __S_ISTYPE((mode), S_IFIFO)


#define __S_IREAD  0400 /* Read by owner.  */
#define __S_IWRITE 0200 /* Write by owner.  */
#define __S_IEXEC  0100 /* Execute by owner.  */

#define S_IRUSR __S_IREAD  /* Read by owner.  */
#define S_IWUSR __S_IWRITE /* Write by owner.  */
#define S_IXUSR __S_IEXEC  /* Execute by owner.  */
/* Read, write, and execute by owner.  */
#define S_IRWXU (__S_IREAD | __S_IWRITE | __S_IEXEC)

#define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */
/* Read, write, and execute by group.  */
#define S_IRWXG (S_IRWXU >> 3)

#define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
/* Read, write, and execute by others.  */
#define S_IRWXO (S_IRWXG >> 3)


#include <udefs.h>

typedef uint16_t mode_t;
typedef uint32_t ino_t;
typedef uint32_t dev_t;
typedef uint16_t nlink_t;
typedef uint16_t uid_t;
typedef uint16_t gid_t;
typedef uint32_t time_t;

struct stat {
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


#endif /* !__USER_INCLUDE_USTAT_H__ */