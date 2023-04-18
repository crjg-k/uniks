#include <ulib.h>
#include <usyscall.h>


int main()
{
	char buf = 0;
	while (buf != '~') {
		read(&buf, 1);
		write(&buf, 1);
	}

	exit(0);
}