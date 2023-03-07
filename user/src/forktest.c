#include <ulib.h>

char info[] = "initproc 1\n";


int main()
{
	if (fork() == 0) {
		info[9] = '2';
		if (fork() == 0) {
			info[9] = '3';
			if (fork() == 0) {
				info[9] = '4';
			}
		}
	}
	while (1) {
		write(info, 11);
	};
}