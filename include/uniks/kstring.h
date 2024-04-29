#ifndef __LIBS_KSTRING_H__
#define __LIBS_KSTRING_H__


#include <uniks/defs.h>


size_t strlen(const char *s);
size_t strnlen(const char *s, size_t len);

char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t len);

int32_t strcmp(const char *s1, const char *s2);
int32_t strncmp(const char *s1, const char *s2, size_t n);

void *memset(void *s, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
int32_t memcmp(const void *v1, const void *v2, size_t n);

int32_t is_separator(char c, const char *tok);
char *strsep(const char *str, const char *tok);
char *strrsep(const char *str, const char *tok);


#endif /* !__LIBS_KSTRING_H__ */