#include <udefs.h>
#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


char filename[] = "/bin/open";
char *tar_argv[] = {filename, NULL};

int main(int argc, char *argv[], char *envp[])
{
	open("/dev/tty0", O_RDONLY);
	int fd = open("/dev/tty0", O_WRONLY);
	dup(fd);

	execve(filename, tar_argv, envp);
}