#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


#define BUFSIZE 4096
unsigned char buf[BUFSIZE];

int main()
{
	int fd = open("./bin/fib", O_RDONLY);
	for (int i = 0; i < 5; i++) {
		int cnt = read(fd, buf, 1024);
		for (int i = 0; i < cnt; i++) {
			printf("%x ", buf[i]);
			if ((i + 1) % 16 == 0)
				printf("\n");
		}
		printf("==============\n");
	}
}