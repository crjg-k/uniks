#ifndef __USER_INCLUDE_ULIB_H__
#define __USER_INCLUDE_ULIB_H__


#include <stdarg.h>
#include <udefs.h>


// basic C enviroment
void _start(int argc, char *argv[], char *envp[]);
int atoi(const char *str);


#endif /* !__USER_INCLUDE_ULIB_H__ */