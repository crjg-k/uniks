#ifdef UNIKS
	#include <ulib.h>
	#include <usyscall.h>
#else
	#include <stdio.h>
#endif

unsigned char data[4096];

int main()
{
	read(3, data, 4096);
	for (int i = 0; i < 4096; i++) {
		printf("%x ", data[i]);
		if ((i + 1) % 16 == 0) {
			printf("\n");
		}
	}
	// data[0x89] = 0x64;
	// int res = write(3, data, 4096);
	// printf("%d\n", res);
}