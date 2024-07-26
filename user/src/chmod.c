#include <ufcntl.h>
#include <ulib.h>
#include <ustat.h>
#include <ustdio.h>
#include <usyscall.h>


void print_usage(const char *program_name)
{
	printf("Usage: %s <mode> <file>\n", program_name);
	printf("Example: %s 755 example.txt\n", program_name);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		print_usage(argv[0]);
		_exit(-1);
	}

	mode_t mode;
	struct stat st;
	char flag;

	if (argv[1][0] == '+' or argv[1][0] == '-') {
		mode = strtol(argv[1] + 1, NULL, 8);
		flag = argv[1][0];
	} else {
		mode = strtol(argv[1], NULL, 8);
		flag = '\0';
	}

	if (mode < 0 or mode > 0777) {
		fprintf(STDERR_FILENO,
			"chmod: parameter `mode` is illegality\n");
		_exit(-1);
	}

	for (int i = 2; i < argc; i++) {
		if (flag != '\0') {
			if (stat(argv[i], &st) < 0) {
				fprintf(STDERR_FILENO, "chmod: Cannot stat '%s'\n",
					argv[i]);
				continue;
			}
			if (flag == '+')
				st.st_mode |= mode;
			else if (flag == '-')
				st.st_mode &= ~mode;
			else
				_exit(-2);
			mode = st.st_mode;
		}

		if (chmod(argv[i], mode) < 0) {
			fprintf(STDERR_FILENO, "Error: chmod '%s' failed\n",
				argv[i]);
		}
	}

	return 0;
}
