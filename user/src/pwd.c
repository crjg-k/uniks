#include <udefs.h>
#include <ulimits.h>
#include <ustdio.h>
#include <usyscall.h>


char buf[PATH_MAX];

int main(int argc, char *argv[])
{
	printf("%s\n", getcwd(buf, PATH_MAX));
}
