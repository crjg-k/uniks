#ifndef __USER_INCLUDE_USTRING_H__
#define __USER_INCLUDE_USTRING_H__


#include <udefs.h>


unsigned long strlen(const char *s);
unsigned long strnlen(const char *s, unsigned long len);

char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, unsigned long len);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, unsigned long n);

void *memset(void *s, char c, unsigned long n);
void *memcpy(void *dst, const void *src, unsigned long n);
int memcmp(const void *v1, const void *v2, unsigned long n);

char *strchr(const char *s, char c);


#endif /* !__USER_INCLUDE_USTRING_H__ */