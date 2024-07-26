#include <ufcntl.h>
#include <ustat.h>
#include <ustdio.h>
#include <usyscall.h>


void print_usage(const char *program_name)
{
	printf("Usage: %s {-s} <oldpath> <newpath>\n", program_name);
	printf("Example(hard link): %s oldfile newfile\n", program_name);
	printf("Example(soft link): %s -s oldfile newfile\n", program_name);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		print_usage(argv[0]);
		_exit(-1);
	}

	if (*argv[1] == '-' and *(argv[1] + 1) == 's') {
		// soft link(symbolic link)
		symlink(argv[2], argv[3]);
		return 0;
	}

	if (link(argv[1], argv[2]) < 0)
		fprintf(STDERR_FILENO, "link %s %s: failed\n", argv[1],
			argv[2]);

	return 0;
}
