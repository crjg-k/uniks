#include <udefs.h>
#include <ufcntl.h>
#include <ulimits.h>
#include <umalloc.h>
#include <ustdio.h>
#include <ustring.h>
#include <usyscall.h>


// Parsed command representation
enum cmdtype {
	EXEC = 1,
	REDIR,
	PIPE,
	LIST,
	BACK,
};

char cmdbuf[LINE_MAX], envpath[LINE_MAX], *cmd_start, *envpath_end = NULL;
char **_envp;

struct cmd {
	int type;
};

struct execcmd {
	int type;
	char *argv[MAXARGS];
	char *eargv[MAXARGS];
};

struct redircmd {
	int type;
	struct cmd *cmd;
	char *file;
	char *efile;
	int mode;
	int fd;
};

struct pipecmd {
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct listcmd {
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct backcmd {
	int type;
	struct cmd *cmd;
};

void panic(char *s)
{
	fprintf(STDERR_FILENO, "ksh: \e[31m%s\e[0m\n", s);
	_exit(-1);
}

// Fork but panics on failure.
int fork1()
{
	int pid;

	pid = fork();
	if (pid == -1)
		panic("fork");
	return pid;
}


// PAGEBREAK!
// Constructors

struct cmd *execcmd(void)
{
	struct execcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = EXEC;
	return (struct cmd *)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, char *efile, int mode,
		     int fd)
{
	struct redircmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = REDIR;
	cmd->cmd = subcmd;
	cmd->file = file;
	cmd->efile = efile;
	cmd->mode = mode;
	cmd->fd = fd;
	return (struct cmd *)cmd;
}

struct cmd *pipecmd(struct cmd *left, struct cmd *right)
{
	struct pipecmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = PIPE;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd *)cmd;
}

struct cmd *listcmd(struct cmd *left, struct cmd *right)
{
	struct listcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = LIST;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd *)cmd;
}

struct cmd *backcmd(struct cmd *subcmd)
{
	struct backcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = BACK;
	cmd->cmd = subcmd;
	return (struct cmd *)cmd;
}


// PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int gettoken(char **ps, char *es, char **q, char **eq)
{
	char *s = *ps;
	int ret, state = 0;

	while (s < es and strchr(whitespace, *s))
		s++;
	if (q)
		*q = s;
	ret = *s;
	switch (*s) {
	case '\0':
		break;
	case '|':
	case '(':
	case ')':
	case ';':
	case '&':
	case '<':
		s++;
		break;
	case '>':
		s++;
		if (*s == '>') {
			ret = '+';
			s++;
		}
		break;
	default:
		/**
		 * @brief Introduce state machine here to identify tokens
		 * surrounded by "".
		 */
		ret = 'a';
		while (s < es) {
			if (state == 0) {
				if (strchr(whitespace, *s) or
				    strchr(symbols, *s))
					break;
				if (*s == '"')
					state = 1;
			} else if (state == 1) {
				ret = 'b';
				if (*s == '"')
					state = 0;
			} else
				panic("BUG");
			++s;
		}
		break;
	}
	if (state == 1)
		panic("syntax error - missing '\"'");

	if (eq)
		*eq = s;

	while (s < es and strchr(whitespace, *s))
		s++;
	*ps = s;
	return ret;
}

int peek(char **ps, char *es, char *toks)
{
	char *s = *ps;
	while (s < es and strchr(whitespace, *s))
		s++;
	*ps = s;
	return *s and strchr(toks, *s);
}

void handle_quotmark(char **q, char **eq)
{
	if (q == NULL or eq == NULL)
		panic("BUG");
	char *buf = malloc(MAXARGLEN), *dst = buf;
	unsigned int n = 0;
	while (*q < *eq) {
		if (**q == '\"') {
			(*q)++;
			continue;
		}
		*dst++ = *(*q)++;
		n++;
	}
	*q = buf;
	*eq = *q + n;
}

struct cmd *parsepipe(char **, char *);

struct cmd *parseline(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parsepipe(ps, es);
	while (peek(ps, es, "&")) {
		gettoken(ps, es, NULL, NULL);
		cmd = backcmd(cmd);
	}
	if (peek(ps, es, ";")) {
		gettoken(ps, es, NULL, NULL);
		cmd = listcmd(cmd, parseline(ps, es));
	}
	return cmd;
}

struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es)
{
	int tok1, tok2;
	char *q, *eq;

	while (peek(ps, es, "<>")) {
		tok1 = gettoken(ps, es, NULL, NULL);
		tok2 = gettoken(ps, es, &q, &eq);
		if (tok2 != 'a' and tok2 != 'b')
			panic("missing file for redirection");
		if (tok2 == 'b')
			handle_quotmark(&q, &eq);

		switch (tok1) {
		case '<':
			cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
			break;
		case '>':
			cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREAT | O_TRUNC,
				       1);
			break;
		case '+':   // >>
			cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREAT, 1);
			break;
		}
	}
	return cmd;
}

struct cmd *parseblock(char **ps, char *es)
{
	struct cmd *cmd;

	if (!peek(ps, es, "("))
		panic("parseblock");
	gettoken(ps, es, NULL, NULL);
	cmd = parseline(ps, es);
	if (!peek(ps, es, ")"))
		panic("syntax error - missing ')'");
	gettoken(ps, es, NULL, NULL);
	cmd = parseredirs(cmd, ps, es);
	return cmd;
}

struct cmd *parseexec(char **ps, char *es)
{
	char *q, *eq;
	int tok, argc;
	struct execcmd *cmd;
	struct cmd *ret;

	if (peek(ps, es, "("))
		return parseblock(ps, es);

	ret = execcmd();
	cmd = (struct execcmd *)ret;

	argc = 0;
	ret = parseredirs(ret, ps, es);
	while (!peek(ps, es, "|)&;")) {
		if ((tok = gettoken(ps, es, &q, &eq)) == 0)
			break;
		if (tok != 'a' and tok != 'b')
			panic("syntax error");
		if (tok == 'b')
			handle_quotmark(&q, &eq);

		cmd->argv[argc] = q;
		cmd->eargv[argc] = eq;
		argc++;
		if (argc >= MAXARGS)
			panic("too many args");
		ret = parseredirs(ret, ps, es);
	}
	cmd->argv[argc] = NULL;
	cmd->eargv[argc] = NULL;
	return ret;
}

struct cmd *parsepipe(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parseexec(ps, es);
	if (peek(ps, es, "|")) {
		gettoken(ps, es, NULL, NULL);
		cmd = pipecmd(cmd, parsepipe(ps, es));
	}
	return cmd;
}

// NUL-terminate all the counted strings.
struct cmd *nulterminate(struct cmd *cmd)
{
	int i;
	struct backcmd *bcmd;
	struct execcmd *ecmd;
	struct listcmd *lcmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if (cmd == NULL)
		return NULL;

	switch (cmd->type) {
	case EXEC:
		ecmd = (struct execcmd *)cmd;
		for (i = 0; ecmd->argv[i]; i++)
			*ecmd->eargv[i] = 0;
		break;

	case REDIR:
		rcmd = (struct redircmd *)cmd;
		nulterminate(rcmd->cmd);
		*rcmd->efile = 0;
		break;

	case PIPE:
		pcmd = (struct pipecmd *)cmd;
		nulterminate(pcmd->left);
		nulterminate(pcmd->right);
		break;

	case LIST:
		lcmd = (struct listcmd *)cmd;
		nulterminate(lcmd->left);
		nulterminate(lcmd->right);
		break;

	case BACK:
		bcmd = (struct backcmd *)cmd;
		nulterminate(bcmd->cmd);
		break;
	}
	return cmd;
}

struct cmd *parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = parseline(&s, es);
	peek(&s, es, "");
	if (s != es) {
		fprintf(STDERR_FILENO, "ksh: Leftovers: '%s'\n", s);
		panic("syntax error");
	}
	nulterminate(cmd);
	return cmd;
}

// Execute cmd. Never returns.
void runcmd(struct cmd *cmd)
{
	int p[2];
	struct backcmd *bcmd;
	struct execcmd *ecmd;
	struct listcmd *lcmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if (cmd == NULL)
		_exit(-1);

	switch (cmd->type) {
	default:
		panic("runcmd");

	case EXEC:
		ecmd = (struct execcmd *)cmd;
		if (ecmd->argv[0] == NULL)
			_exit(-1);
		execve(ecmd->argv[0], ecmd->argv, _envp);

		/**
		 * @brief It means that execve() failed with original path, try
		 * again with the path where the environment variable is
		 * located.
		 */
		if (ecmd->argv[0][0] != '.' and ecmd->argv[0][0] != '/' and
		    envpath_end) {
			strcpy(envpath_end, ecmd->argv[0]);
			execve(envpath, ecmd->argv, _envp);
		}
		fprintf(STDERR_FILENO, "ksh: Execve '%s' failed\n",
			ecmd->argv[0]);
		break;

	case REDIR:
		rcmd = (struct redircmd *)cmd;
		close(rcmd->fd);
		if (open(rcmd->file, rcmd->mode) < 0) {
			fprintf(STDERR_FILENO,
				"ksh: Cannot access '%s': No such file or directory\n",
				rcmd->file);
			_exit(-1);
		}
		runcmd(rcmd->cmd);
		break;

	case LIST:
		lcmd = (struct listcmd *)cmd;
		if (fork1() == 0)
			runcmd(lcmd->left);
		wait(NULL);
		runcmd(lcmd->right);
		break;

	case PIPE:
		pcmd = (struct pipecmd *)cmd;
		if (pipe(p) < 0)
			panic("pipe");
		if (fork1() == 0) {
			close(1);
			dup(p[1]);
			close(p[0]);
			close(p[1]);
			runcmd(pcmd->left);
		}
		if (fork1() == 0) {
			close(0);
			dup(p[0]);
			close(p[0]);
			close(p[1]);
			runcmd(pcmd->right);
		}
		close(p[0]);
		close(p[1]);
		wait(NULL);
		wait(NULL);
		break;

	case BACK:
		bcmd = (struct backcmd *)cmd;
		if (fork1() == 0)
			runcmd(bcmd->cmd);
		break;
	}
	_exit(0);
}

int trim(char *cmd, int len)
{
	cmd_start = cmd;
	while (strchr(whitespace, *cmd_start))
		++cmd_start;
	while (strchr(whitespace, cmd[len - 1]))
		--len;
	cmd[len] = '\0';
	return len;
}

char cwd[PATH_MAX];
int getcmd(char *buf, int nbuf)
{
	printf("\e[35m[root@uniks %s]# \e[0m", getcwd(cwd, PATH_MAX));
	int len = getline(buf, nbuf);
	if (buf[0] == '\0')   // EOF
		return -1;
	return trim(cmdbuf, len);
}

void set_envpath()
{
	for (int i = 0; _envp[i]; i++) {
		if (strncmp(_envp[i], "PATH=", 5) == 0) {
			strcpy(envpath, _envp[i] + 5);
			envpath_end = envpath + strlen(envpath);
			*envpath_end++ = '/';
		}
	}
}

int main(int argc, char *argv[], char *envp[])
{
	_envp = envp;
	set_envpath();
	int len;

	// Read and run input commands.
	while ((len = getcmd(cmdbuf, LINE_MAX)) >= 0) {
		if (!len)
			continue;
		if (strcmp(cmd_start, "cd") == 0) {
			if (chdir("/root") < 0)
				fprintf(STDERR_FILENO,
					"ksh: Cannot cd '/root'\n");
			continue;
		} else if (strncmp(cmd_start, "cd ", 3) == 0) {
			// Chdir must be called by the parent, not the child.
			trim(cmd_start + 3, len - 3);
			if (chdir(cmd_start) < 0)
				fprintf(STDERR_FILENO, "ksh: Cannot cd '%s'\n",
					cmd_start);
			continue;
		} else if (strncmp(cmd_start, "clear", 5) == 0) {
			printf("\33[H\33[2J\33[3J");
			continue;
		} else if (strncmp(cmd_start, "exit", 4) == 0)
			_exit(0);

		/**
		 * @brief This is a pretty clever way to do lexical parsing in
		 * the child process after fork(). We could using malloc() to
		 * dynamically allocate memory without free() and there is no
		 * need to worry about memory leakage issues, because when the
		 * child process finishes executing, the OS will recycle all
		 * the virtual address space of the process when exiting!
		 */
		if (fork1() == 0)
			runcmd(parsecmd(cmd_start));
		wait(NULL);
	}
	_exit(-1);
}