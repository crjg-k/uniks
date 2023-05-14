#ifdef UNIKS
	#include <ulib.h>
	#include <usyscall.h>
#else
	#include <stdio.h>
#endif


int main(int argc, char *argv[], char *envp[])
{
	for (int i = 0; envp[i]; i++) {
		printf("%s\n", envp[i]);
	}
}