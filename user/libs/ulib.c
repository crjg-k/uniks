#include <limits.h>
#include <ulib.h>
#include <ustdio.h>
#include <usyscall.h>


extern int main(int argc, char *argv[], char *envp[]);

void _start(int argc, char *argv[], char *envp[])
{
	ustdioinit();
	int exitcode = main(argc, argv, envp);
	_exit(exitcode);
}


long strtol(const char *str, char **endptr, int base)
{
	const char *ptr = str;
	long res = 0, sign = 1;

	// Skip leading whitespace
	while (isspace(*ptr)) {
		ptr++;
	}

	// Handle optional sign
	if (*ptr == '-') {
		sign = -1;
		ptr++;
	} else if (*ptr == '+') {
		ptr++;
	}

	// Determine the base if not specified
	if (base == 0) {
		if (*ptr == '0') {
			if (tolower(*(ptr + 1)) == 'x') {
				base = 16;
				ptr += 2;
			} else {
				base = 8;
				ptr++;
			}
		} else {
			base = 10;
		}
	} else if (base == 16 and *ptr == '0' and tolower(*(ptr + 1)) == 'x') {
		ptr += 2;
	}

	// Check for valid base range
	if (base < 2 or base > 36) {
		if (endptr)
			*endptr = (char *)str;
		return 0;
	}

	// Convert each character
	while (*ptr) {
		int digit;
		if (isdigit(*ptr)) {
			digit = *ptr - '0';
		} else if (isalpha(*ptr)) {
			digit = tolower(*ptr) - 'a' + 10;
		} else
			break;

		if (digit >= base)
			break;

		res = res * base + digit;
		ptr++;
	}

	if (endptr)
		*endptr = (char *)ptr;

	return sign * res;
}

int atoi(const char *nptr)
{
	return (int)strtol(nptr, NULL, 10);
}

uintptr_t last_brk = 0;
void *sbrk(uintptr_t inc)
{
	if (!last_brk)
		last_brk = brk(NULL);

	uintptr_t p = last_brk;

	last_brk = brk((void *)(p + inc));

	if (last_brk < p + inc)
		return NULL;

	return (void *)p;
}