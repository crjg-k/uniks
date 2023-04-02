#include <ulib.h>

char info[] = "initproc 1\n";


int main()
{
#define NPROC 7
	for (int i = 0; i < NPROC; i++) {
		if (fork() == 0) {
			goto output;
		}
	}
output:
	info[9] = getpid() + '0';
	while (1) {
		write(info, 11);
	};
}