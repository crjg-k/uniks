#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


#define BUFSIZE 1024
unsigned char buf[BUFSIZE];

int main()
{
	int fd = open("/dev/vda0", O_RDONLY);
	for (int i = 0; i < 5; i++) {
		int cnt = read(fd, buf, BUFSIZE);
		for (int i = 0; i < cnt; i++) {
			printf("%02X ", buf[i]);
			if ((i + 1) % 16 == 0)
				printf("\n");
		}
		printf("==============\n");
	}
}