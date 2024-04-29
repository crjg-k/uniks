#ifndef __USER_INCLUDE_UDIRENT_H__
#define __USER_INCLUDE_UDIRENT_H__


#define MAX_NAME_LEN (255)

struct dirent {
	uint32_t d_inode;  /* 32-bit Inode number */
	uint16_t d_reclen; /* Size of this dirent */
	uint8_t d_namelen; /* Name length */
	uint8_t d_filetype;
	char d_name[MAX_NAME_LEN + 1]; /* Filename (null-terminated) */
} __packed;


#endif /* !__USER_INCLUDE_UDIRENT_H__ */