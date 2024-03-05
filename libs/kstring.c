#include <uniks/kstring.h>

size_t strlen(const char *s)
{
	size_t cnt = 0;
	while (*s++ != 0)
		cnt++;
	return cnt;
}
size_t strnlen(const char *s, size_t len)
{
	size_t cnt = 0;
	while (cnt < len and *s++ != 0)
		cnt++;
	return cnt;
}

char *strcpy(char *dst, const char *src)
{
	char *p = dst;
	while ((*p++ = *src++) != 0)
		/* nothing */;
	return dst;
}
char *strncpy(char *dst, const char *src, size_t len)
{
	char *p = dst;
	while (len > 0) {
		if ((*p = *src) != 0)
			src++;
		else
			return dst;
		p++, len--;
	}
	*(--p) = '\0';
	return dst;
}

int32_t strcmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' and *s1 == *s2) {
		s1++, s2++;
	}
	return (int)((uint8_t)*s1 - (uint8_t)*s2);
}
int32_t strncmp(const char *s1, const char *s2, size_t n)
{
	while (n > 0 and *s1 != '\0' and *s1 == *s2) {
		n--, s1++, s2++;
	}
	return (n == 0) ? 0 : (int)((uint8_t)*s1 - (uint8_t)*s2);
}

void *memset(void *s, char c, size_t n)
{
	char *p = s;
	while (n-- > 0) {
		*p++ = c;
	}
	return s;
}

void *memcpy(void *dst, const void *src, size_t n)
{
	const char *s = src;
	char *d = dst;
	while (n-- > 0) {
		*d++ = *s++;
	}
	return dst;
}

int32_t memcmp(const void *v1, const void *v2, size_t n)
{
	const char *s1 = (const char *)v1;
	const char *s2 = (const char *)v2;
	while (n-- > 0) {
		if (*s1 != *s2)
			return (int32_t)((uint8_t)*s1 - (uint8_t)*s2);
		s1++, s2++;
	}
	return 0;
}
