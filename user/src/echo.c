#ifdef UNIKS
	#include <ulib.h>
	#include <usyscall.h>
#else
	#include <stdio.h>
#endif


int main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		printf("%s", argv[i]);
		if (i < argc - 1)
			printf(" ");
	}
	printf("\n");
}