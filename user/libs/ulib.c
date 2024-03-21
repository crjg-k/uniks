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

#define is_digit(c) ((c) >= '0' and (c) <= '9')
#define is_space(c) ((c) == ' ' or (c) == '\n' or (c) == '\t')

int atoi(const char *str)
{
	if (str == NULL or *str == '\0') {
		return 0;
	}
	while (is_space(*str)) {
		str++;
	}
	int flag = 1;
	if (*str == '+') {
		flag = 1;
		str++;
	} else if (*str == '-') {
		flag = -1;
		str++;
	}
	long ret = 0;
	while (is_digit(*str)) {
		ret = ret * 10 + (*str - '0') * flag;
		if (ret < INT_MIN or ret > INT_MAX) {
			return 0;
		}
		str++;
	}
	if (*str == '\0')
		return (int)ret;
	else
		return (int)ret;
}
