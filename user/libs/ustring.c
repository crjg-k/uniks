#include <ustring.h>


unsigned long strlen(const char *s)
{
	unsigned long cnt = 0;
	while (*s++ != 0)
		cnt++;
	return cnt;
}

unsigned long strnlen(const char *s, unsigned long len)
{
	unsigned long cnt = 0;
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

char *strncpy(char *dst, const char *src, unsigned long len)
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

int strcmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' and *s1 == *s2) {
		s1++, s2++;
	}
	return (int)((unsigned char)*s1 - (unsigned char)*s2);
}

int strncmp(const char *s1, const char *s2, unsigned long n)
{
	while (n > 0 and *s1 != '\0' and *s1 == *s2) {
		n--, s1++, s2++;
	}
	return (n == 0) ? 0 : (int)((unsigned char)*s1 - (unsigned char)*s2);
}

void *memset(void *s, char c, unsigned long n)
{
	char *p = s;
	while (n-- > 0) {
		*p++ = c;
	}
	return s;
}

void *memcpy(void *dst, const void *src, unsigned long n)
{
	const char *s = src;
	char *d = dst;
	while (n-- > 0) {
		*d++ = *s++;
	}
	return dst;
}

int memcmp(const void *v1, const void *v2, unsigned long n)
{
	const char *s1 = (const char *)v1;
	const char *s2 = (const char *)v2;
	while (n-- > 0) {
		if (*s1 != *s2)
			return (int)((unsigned char)*s1 - (unsigned char)*s2);
		s1++, s2++;
	}
	return 0;
}

char *strchr(const char *s, char c)
{
	for (; *s; s++)
		if (*s == c)
			return (char *)s;
	return NULL;
}