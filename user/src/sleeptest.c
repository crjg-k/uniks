#include <ulib.h>
#include <usyscall.h>

char info[] = "parent proc: child 0 exit with status: 0\n";


int main()
{
	int status;
#define NPROC 8
	for (int i = 1; i <= NPROC; i++) {
		if (fork() == 0) {
			msleep(i * 1000);
			exit(i + 1);
		}
	}
	for (int i = 1; i <= NPROC; i++) {
		waitpid(i + 1, &status);
		info[19] = i + 1 + '0';
		info[39] = status + '0';
		write(info, 41);
	}
	exit(0);
}