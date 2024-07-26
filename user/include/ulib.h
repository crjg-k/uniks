#ifndef __USER_INCLUDE_ULIB_H__
#define __USER_INCLUDE_ULIB_H__


#include <stdarg.h>
#include <udefs.h>


#define isdigit(c) ((c) >= '0' and (c) <= '9')
#define isspace(c) ((c) == ' ' or (c) == '\n' or (c) == '\t')
#define tolower(c) (c | 0b00100000)
#define toupper(c) (c & 0b11011111)
#define isalpha(c) (((c) >= 'A' and (c) <= 'Z') or ((c) >= 'a' and (c) <= 'z'))


// basic C enviroment
void _start(int argc, char *argv[], char *envp[]);
long strtol(const char *str, char **endptr, int base);
int atoi(const char *nptr);
void *sbrk(uintptr_t inc);


#endif /* !__USER_INCLUDE_ULIB_H__ */