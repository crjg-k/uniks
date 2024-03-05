#ifndef __USER_INCLUDE_USTDIO_H__
#define __USER_INCLUDE_USTDIO_H__


/* Standard file descriptors. */
#define STDIN_FILENO  0 /* Standard input. */
#define STDOUT_FILENO 1 /* Standard output. */
#define STDERR_FILENO 2 /* Standard error output. */


// std libc function
void init_io();
int printf(const char *fmt, ...);
char getchar();


#endif /* !__USER_INCLUDE_USTDIO_H__ */