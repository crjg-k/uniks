#ifndef __USER_INCLUDE_ULIB_H__
#define __USER_INCLUDE_ULIB_H__


#include <stdarg.h>
#include <udefs.h>


// basic C enviroment
void _start(int argc, char *argv[], char *envp[]);
int atoi(const char *str);
void *sbrk(uintptr_t inc);


#endif /* !__USER_INCLUDE_ULIB_H__ */