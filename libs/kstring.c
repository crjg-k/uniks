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
	size_t i;

	for (i = 0; i < len and src[i] != '\0'; i++)
		dst[i] = src[i];
	for (; i < len; i++)
		dst[i] = '\0';

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

int32_t is_separator(char c, const char *tok)
{
	for (; *tok; tok++) {
		if (c == *tok)
			return 1;
	}
	return 0;
}

// Get the first delimiter
char *strsep(const char *str, const char *tok)
{
	char *ptr = (char *)str;
	while (1) {
		if (is_separator(*ptr, tok))
			return ptr;
		if (*ptr++ == '\0')
			return NULL;
	}
}

// Get the last delimiter
char *strrsep(const char *str, const char *tok)
{
	char *last = NULL;
	char *ptr = (char *)str;
	while (1) {
		if (is_separator(*ptr, tok))
			last = ptr;
		if (*ptr++ == '\0')
			return last;
	}
}