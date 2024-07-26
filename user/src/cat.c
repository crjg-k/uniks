#include <udefs.h>
#include <udirent.h>
#include <ufcntl.h>
#include <ulimits.h>
#include <umalloc.h>
#include <ustat.h>
#include <ustdio.h>
#include <ustring.h>
#include <usyscall.h>


char buf[SECTORSIZE];

void cat(int fd)
{
	int n;

	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		if (write(STDOUT_FILENO, buf, n) != n) {
			fprintf(STDERR_FILENO, "cat: write error\n");
			_exit(-1);
		}
	}
	if (n < 0) {
		fprintf(STDERR_FILENO, "cat: read error\n");
		_exit(-1);
	}
}

int main(int argc, char *argv[])
{
	int fd;

	if (argc <= 1) {
		cat(0);
		_exit(0);
	}

	for (int i = 1; i < argc; i++) {
		if ((fd = open(argv[i], 0)) < 0) {
			fprintf(STDERR_FILENO,
				"cat: cannot access '%s': no such file or directory\n",
				argv[i]);
			_exit(-1);
		}
		cat(fd);
		close(fd);
	}
	_exit(0);
}
