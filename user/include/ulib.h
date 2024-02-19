#ifndef __USER_INCLUDE_ULIB_H__
#define __USER_INCLUDE_ULIB_H__

#include <stdarg.h>
#include <stdint.h>

#ifndef NULL
	#define NULL ((void *)0)
#endif

#ifndef and
	#define and &&
#endif
#ifndef or
	#define or ||
#endif

#ifndef EOF
	#define EOF ((char)-1)
#endif


// basic C enviroment
void _start(int argc, char *argv[], char *envp[]);

// std libc function
int printf(const char *fmt, ...);
char getchar();


#endif /* !__USER_INCLUDE_ULIB_H__ */