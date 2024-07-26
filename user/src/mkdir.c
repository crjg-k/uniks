#include <ufcntl.h>
#include <ustat.h>
#include <ustdio.h>
#include <usyscall.h>


int main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (mkdir(argv[i], S_IRWXU) < 0)
			fprintf(STDERR_FILENO, "Error: mkdir '%s' failed\n",
				argv[i]);
	}
}