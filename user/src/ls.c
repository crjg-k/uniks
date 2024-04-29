#include <udefs.h>
#include <udirent.h>
#include <ufcntl.h>
#include <ulimits.h>
#include <umalloc.h>
#include <ustat.h>
#include <ustdio.h>
#include <ustring.h>
#include <usyscall.h>


int paramset = 0;
#define P_a 0b001
#define P_l 0b010
#define P_i 0b100
#define CheckParam(ch) \
	if (*arg == *#ch) { \
		paramset |= P_##ch; \
		continue; \
	}

void parse_param(char arg[])
{
	while (*++arg) {
		CheckParam(a);
		CheckParam(l);
		CheckParam(i);
		printf("ls: Unknown param '-%c'\n", *arg);
	}
}

char *mod2str(int mode)
{
	static char str[] = "----------";
	switch (mode & 0xf000) {
	case S_IFDIR:
		str[0] = 'd';
		break;
	case S_IFLNK:
		str[0] = 'l';
		break;
	case S_IFCHR:
		str[0] = 'c';
		break;
	case S_IFBLK:
		str[0] = 'b';
	}
	str[1] = (mode & S_IRUSR) ? 'r' : '-';
	str[2] = (mode & S_IWUSR) ? 'w' : '-';
	str[3] = (mode & S_IXUSR) ? 'x' : '-';
	str[4] = (mode & S_IRGRP) ? 'r' : '-';
	str[5] = (mode & S_IWGRP) ? 'w' : '-';
	str[6] = (mode & S_IXGRP) ? 'x' : '-';
	str[7] = (mode & S_IROTH) ? 'r' : '-';
	str[8] = (mode & S_IWOTH) ? 'w' : '-';
	str[9] = (mode & S_IXOTH) ? 'x' : '-';
	return str;
}

void listfile(struct stat *einfo, char fname[])
{
	if (paramset & P_i)
		printf("%8lu  ", einfo->st_ino);
	if (paramset & P_l) {
		printf("%s  ", mod2str(einfo->st_mode));
		printf("%4d  ", einfo->st_nlink);
		printf("%-8s", "root");
		printf("%-8s", "root");
		if (S_ISCHR(einfo->st_mode) or S_ISBLK(einfo->st_mode)) {
			printf("0,%4d  ", einfo->st_rdev);
		} else
			printf("%8ld  ", einfo->st_size);
		printf("%d  ", einfo->st_mtime);
	}

	if (S_ISDIR(einfo->st_mode)) {
		printf("\e[1;34m%-6s\e[0m", fname);
	} else if (S_ISCHR(einfo->st_mode) or S_ISBLK(einfo->st_mode)) {
		printf("\e[1;93m%-6s\e[0m", fname);
	} else if (S_IXUSR & einfo->st_mode) {
		printf("\e[1;92m%-6s\e[0m", fname);
	} else
		printf("%-6s", fname);
}

#define D_LEN 2 * sizeof(struct dirent)
char dentry[D_LEN];
void ls(char dir[])
{
	int fd, i = 0;
	long nread;
	struct stat st;
	struct dirent *d;
	char path[PATH_MAX], *p;

	if ((fd = open(dir, O_RDONLY)) < 0) {
		fprintf(STDERR_FILENO,
			"ls: Cannot access '%s': No such file or directory\n",
			dir);
		return;
	}
	if (fstat(fd, &st) < 0) {
		fprintf(STDERR_FILENO, "ls: Cannot stat '%s'\n", dir);
		close(fd);
		return;
	}

	switch (st.st_mode & 0xf000) {
	default:
		fprintf(STDERR_FILENO, "ls: File mode error\n");
		_exit(-1);
	case S_IFSOCK:
	case S_IFLNK:
	case S_IFREG:
	case S_IFBLK:
	case S_IFCHR:
	case S_IFIFO:
		listfile(&st, dir);
		break;
	case S_IFDIR:
		printf("%s:\n", dir);

		if (strlen(dir) + 1 + MAX_NAME_LEN + 1 > sizeof(path)) {
			fprintf(STDERR_FILENO, "ls: Path too long\n");
			break;
		}
		strcpy(path, dir);
		p = path + strlen(path);
		*p++ = '/';
		while (1) {
			nread = getdents(fd, (void *)dentry, D_LEN);
			if (nread < 0) {
				fprintf(STDERR_FILENO, "ls: Getdents error\n");
				break;
			}
			if (nread == 0)
				break;

			for (long bpos = 0; bpos < nread;) {
				d = (struct dirent *)(dentry + bpos);
				if (!(paramset & P_a) and d->d_name[0] == '.')
					goto inc;

				// output
				sprintf(p, "%s", d->d_name);
				if (stat(path, &st) < 0) {
					fprintf(STDERR_FILENO,
						"ls: Cannot stat '%s'\n", path);
					goto inc;
				}
				listfile(&st, d->d_name);
				printf(((paramset & P_l) or (i++ % 5 == 4))
					       ? "\n"
					       : "  ");
inc:
				bpos += d->d_reclen;
			}
		}
	}
	printf("\n");

	close(fd);
}

int main(int argc, char *argv[])
{
	int has_path = 0;
	for (int i = 1; i < argc; i++)
		if (*argv[i] == '-')
			parse_param(argv[i]);
		else
			has_path++;

	while (--argc)
		if (**++argv != '-')
			ls(*argv);
	if (!has_path)
		ls(".");
}