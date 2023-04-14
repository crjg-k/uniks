#include <ulib.h>
#include <usyscall.h>

char info1[] = "child proc start\n";
char info2[] = "child proc sleep over\n";
char info3[] = "parent proc: child exit status: 0\n";


int main()
{
	int id, status;
	if ((id = fork()) == 0) {
		write(info1, 17);
		msleep(1000);
		write(info2, 22);
	} else {
		waitpid(id, &status);
		info3[32] = status + '0';
		write(info3, 34);
	}
	exit(9);
}