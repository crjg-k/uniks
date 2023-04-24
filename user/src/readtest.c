#include <ulib.h>
#include <usyscall.h>

char parent[] = "parent: k\n";
char child[] = "child: k\n";
int main()
{
	char buf;
	int n, pid = fork();
	while (n = read(0, &buf, 1)) {
		if (!pid) {
			child[7] = buf;
			write(1, child, 9);
		} else {
			parent[8] = buf;
			write(1, parent, 10);
		}
	}
	exit(0);
}