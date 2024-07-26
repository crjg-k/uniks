#include <ufcntl.h>
#include <ustat.h>
#include <ustdio.h>
#include <usyscall.h>


int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (creat(argv[i], S_IRUSR | S_IWUSR) < 0)
			fprintf(STDERR_FILENO, "Error: touch '%s' failed\n",
				argv[i]);
	}
}