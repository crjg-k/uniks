#include <ulib.h>
#include <ustdio.h>
#include <usyscall.h>


extern int main(int argc, char *argv[], char *envp[]);

void _start(int argc, char *argv[], char *envp[])
{
	init_io();
	int exitcode = main(argc, argv, envp);
	exit(exitcode);
}
