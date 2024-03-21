#include <udefs.h>
#include <ufcntl.h>
#include <ulib.h>
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

int main(int argc, char *argv[])
{
	int n;
	if (argc < 2) {
		while (1) {
			scanf("%d", &n);
			printf("%dth fib:%d\n", n, fib(n));
		}
	} else {
		for (int i = 1; i < argc; i++) {
			n = atoi(argv[i]);
			printf("%dth fib:%d\n", n, fib(n));
		}
	}
}