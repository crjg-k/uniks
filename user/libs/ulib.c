#include <ulib.h>

void _start()
{
	extern int main();
	main();
	while (1)
		;
}