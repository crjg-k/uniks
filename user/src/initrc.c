#include <udefs.h>
#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


char filename[] = "/bin/ksh";
char *_argv[] = {filename, NULL};

__noreturn void bootshell(char *argv[], char *envp[])
{
	int pid, wpid;
	int status, i = 0;

	while (1) {
		if ((pid = fork()) < 0) {
			printf("%s: Fork failed %d in initrc\n", argv[0], pid);
			continue;
		}
		if (pid == 0) {
			// child process
			int res = execve(filename, _argv, envp);
			printf("%s: execve %s failed:%d\n", argv[0], filename,
			       res);
			_exit(-1);
		}
		while (1) {
			/**
			 * @brief This call to wait() returns if the shell
			 * exits, or if a parentless process exits.
			 */
			wpid = wait(&status);
			if (wpid == pid) {
				// the shell exited; restart it
				printf("%s: %d, wpid[%d] exit %d\n", argv[0],
				       i++, wpid, status);
				break;
			} else if (wpid < 0) {
				printf("%s: wait() returned an error\n",
				       argv[0]);
				_exit(-2);
			} else {
				// it was a parentless process; do nothing
				printf("%s: zombie process[%d] exit %d\n",
				       argv[0], wpid, status);
			}
		}
	}
}

int main(int argc, char *argv[], char *envp[])
{
	int fd = open("/dev/tty0", O_RDWR);
	dup(fd);
	dup(fd);

	printf("%s:", argv[0]);
	for (int i = 1; i < argc; i++)
		printf(" %s", argv[i]);

	printf("\n");

	bootshell(argv, envp);
}