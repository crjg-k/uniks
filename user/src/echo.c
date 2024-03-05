#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>
 

int main(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++) {
		printf("%s", argv[i]);
		if (i < argc - 1)
			printf(" ");
	}
	printf("\n");
}