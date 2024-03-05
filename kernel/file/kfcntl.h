#ifndef __KERNEL_FILE_KFCNTL_H__
#define __KERNEL_FILE_KFCNTL_H__


#include <uniks/defs.h>

#define O_RDONLY 0x000
#define O_WRONLY 0x001
#define O_RDWR	 0x002
#define O_CREAT	 0x040
#define O_TRUNC	 0x200
#define O_APPEND 0x400


#define READABLE(flag) \
	((get_var_bit(flag, 0b11) == 0) or get_var_bit(flag, O_RDWR))
#define WRITEABLE(flag) \
	(get_var_bit(flag, O_WRONLY) or get_var_bit(flag, O_RDWR))


#endif /* !__KERNEL_FILE_KFCNTL_H__ */