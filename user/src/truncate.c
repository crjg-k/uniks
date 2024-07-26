#include <ufcntl.h>
#include <ulib.h>
#include <ustat.h>
#include <ustdio.h>
#include <ustring.h>
#include <usyscall.h>


void print_usage(const char *program_name)
{
	printf("Usage: %s -s <size> <file>\n", program_name);
	printf("Example: %s -s 4096 example.txt\n", program_name);
}

int main(int argc, char *argv[])
{
	if ((argc < 4) or (strcmp(argv[1], "-s") != 0)) {
		print_usage(argv[0]);
		_exit(-1);
	}

	char *size_str = argv[2], flag;
	long size;
	struct stat st;
	if (*size_str == '+' or *size_str == '-') {
		size = strtol(size_str + 1, NULL, 10);
		flag = *size_str++;
	} else {
		size = strtol(size_str, NULL, 10);
		flag = '\0';
	}

	for (int i = 3; i < argc; i++) {
		if (flag != '\0') {
			if (stat(argv[i], &st) < 0) {
				fprintf(STDERR_FILENO,
					"chmod: Cannot stat '%s'\n", argv[i]);
				continue;
			}
			if (flag == '+')
				st.st_size += size;
			else if (flag == '-')
				st.st_size -= size;
			else
				_exit(-2);
			size = st.st_size;
		}

		if (truncate(argv[i], size) < 0) {
			fprintf(STDERR_FILENO, "Error: truncate '%s' failed\n",
				argv[i]);
		}
	}

	return 0;
}
