#ifdef UNIKS
	#include <ulib.h>
	#include <usyscall.h>
#else
	#include <stdio.h>
#endif

int main()
{
	for (int i = 0; i < 10000; i++) {
		printf("%d\t", i);
	}
}