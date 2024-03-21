#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


#define BUFSIZE 1024
unsigned char buf[BUFSIZE];

int main(int argc, char *argv[], char *envp[])
{
	for (int i = 0; i < argc; i++)
		printf("%s ", argv[i]);

	printf("\n");
	for (int i = 0; envp[i]; i++)
		printf("%s\n", envp[i]);

	printf("\n");
}