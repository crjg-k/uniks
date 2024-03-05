#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


int main(int argc, char *argv[], char *envp[])
{
	for (int i = 0; envp[i]; i++) {
		printf("%s\n", envp[i]);
	}
}