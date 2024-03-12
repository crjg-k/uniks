#include <udefs.h>
#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


int fib(int n)
{
	if ((n == 1) or (n == 2)) {
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

int main()
{
	int n;
	while (1) {
		scanf("%d", &n);
		printf("%dth fib:%d\n", n, fib(n));
	}
}