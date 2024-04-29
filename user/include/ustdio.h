#ifndef __USER_INCLUDE_USTDIO_H__
#define __USER_INCLUDE_USTDIO_H__


/* Standard file descriptors. */
#define STDIN_FILENO  0 /* Standard input. */
#define STDOUT_FILENO 1 /* Standard output. */
#define STDERR_FILENO 2 /* Standard error output. */


// === stdio init ===
void ustdioinit();

// === output ===
int sprintf(char *buf, const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
int printf(const char *fmt, ...);

// === input ===
char getchar();
int getline(char *s, int n);
int scanf(const char *fmt, ...);
static void ibuffer_init();


#endif /* !__USER_INCLUDE_USTDIO_H__ */