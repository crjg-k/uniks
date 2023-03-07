#ifndef __USER_INCLUDE_ULIB_H__
#define __USER_INCLUDE_ULIB_H__

#include "usyscall.h"


// basic C enviroment
void _start();


// system calls
int fork();
int write(char *buf, long len);


#endif /* !__USER_INCLUDE_ULIB_H__ */