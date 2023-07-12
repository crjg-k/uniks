#ifndef __ERRNO_H__
#define __ERRNO_H__


/**
 * @brief directly use Linux system corresponding file as below.
 * note: this header file need Linux enviroment path[/usr/include/] in makefile
 */
#if 1
	#include <asm-generic/errno.h>
#endif


#endif /* !__ERRNO_H__ */
