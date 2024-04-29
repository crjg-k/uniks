#include <udefs.h>
#include <ufcntl.h>
#include <ustdio.h>
#include <ustring.h>
#include <usyscall.h>


char buf[512];

void wc(int fd, char *name)
{
	int n, l, w, c, inword;

	l = w = c = 0;
	inword = 0;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		for (int i = 0; i < n; i++) {
			c++;
			if (buf[i] == '\n')
				l++;
			if (strchr(" \r\t\n\v", buf[i]))
				inword = 0;
			else if (!inword) {
				w++;
				inword = 1;
			}
		}
	}
	if (n < 0) {
		fprintf(STDERR_FILENO, "wc: Read error\n");
		_exit(1);
	}
	printf("\t%d\t%d\t%d %s\n", l, w, c, name);
}

int main(int argc, char *argv[])
{
	int fd;

	if (argc <= 1) {
		wc(0, "");
		_exit(0);
	}

	for (int i = 1; i < argc; i++) {
		if ((fd = open(argv[i], 0)) < 0) {
			fprintf(STDERR_FILENO, "wc: Cannot open %s\n", argv[i]);
			_exit(1);
		}
		wc(fd, argv[i]);
		close(fd);
	}
}
