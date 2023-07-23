#ifdef UNIKS
	#include <ulib.h>
	#include <usyscall.h>
#else
	#include <stdio.h>
#endif

int fib(int n)
{
	if ((n == 1) || (n == 2)) {
		return 1;
	}
	int prev = 1;
	int curr = 1;
	for (int i = 3; i <= n; i++) {
		int sum = prev + curr;
		prev = curr;
		curr = sum;
	}
	return curr;
}
int getnum()
{
	char ch;
	int sum = 0;
	while ((ch = getchar()) != '\r') {
		sum *= 10;
		sum += ch - '0';
	}
	return sum;
}
int main()
{
	while (1) {
		int n = getnum();
		printf("\nn%d\n", fib(n));
	}
}