#include <ulib.h>
#include <usyscall.h>
#define BLOCKSIZE 4096

char block[BLOCKSIZE];
char info[] = "0x   ";
char radix[] = "0123456789abcdef";
int main()
{
	for (int i = 0; i < 3; i++) {
		read(0, block, BLOCKSIZE);
		for (int i = 0; i < BLOCKSIZE; i++) {
			info[2] = radix[(block[i] >> 4) & 0xf];
			info[3] = radix[block[i] & 0xf];
			write(1, info, 5);
			if ((i + 1) % 16 == 0)
				write(1, "\n", 1);
		}
		write(1, "\n", 1);
	}
	exit(0);
}