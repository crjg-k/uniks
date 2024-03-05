#include <stdarg.h>
#include <udefs.h>
#include <ufcntl.h>
#include <ustdio.h>
#include <usyscall.h>


static char digits[] = "0123456789abcdef";
char buffer[4096];
int idx, cnt;


void init_io()
{
	int fd = open("/dev/tty0", O_RDWR);
	dup(fd);
	dup(fd);
}

void kputc(char c)
{
	buffer[idx++] = c;
	cnt++;
}

void kputs(const char *str)
{
	char c;
	while ((c = *str++) != '\0')
		kputc(c);
}

static void printint(int xx, int base, int sgn)
{
	char buf[16];
	int i, neg;
	unsigned int x;

	neg = 0;
	if (sgn and xx < 0) {
		neg = 1;
		x = -xx;
	} else {
		x = xx;
	}

	i = 0;
	do {
		buf[i++] = digits[x % base];
	} while ((x /= base) != 0);
	if (neg)
		buf[i++] = '-';

	while (--i >= 0)
		kputc(buf[i]);
}
static void printptr(unsigned long long x)
{
	kputs("0x");
	for (int i = 0; i < (sizeof(unsigned long long) * 2); i++, x <<= 4)
		kputc(digits[x >> (sizeof(unsigned long long) * 8 - 4)]);
}
static int vprintf(const char *fmt, va_list ap)
{
	char *s;
	int c, i, state = 0;
	idx = cnt = 0;

	for (i = 0; fmt[i]; i++) {
		c = fmt[i] & 0xff;
		if (state == 0) {
			if (c == '%') {
				state = '%';
			} else {
				kputc(c);
			}
		} else if (state == '%') {
			if (c == 'd') {
				printint(va_arg(ap, int), 10, 1);
			} else if (c == 'l') {
				printint(va_arg(ap, unsigned long long), 10, 0);
			} else if (c == 'x') {
				printint(va_arg(ap, int), 16, 0);
			} else if (c == 'p') {
				printptr(va_arg(ap, unsigned long long));
			} else if (c == 's') {
				s = va_arg(ap, char *);
				if (s == 0)
					s = "(null)";
				while (*s != 0) {
					kputc(*s);
					s++;
				}
			} else if (c == 'c') {
				kputc(va_arg(ap, unsigned int));
			} else if (c == '%') {
				kputc(c);
			} else {
				// Unknown % sequence.  Print it to draw
				// attention.
				kputc('%');
				kputc(c);
			}
			state = 0;
		}
	}
	return cnt;
}

int printf(const char *fmt, ...)
{
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vprintf(fmt, ap);
	va_end(ap);
	write(STDOUT_FILENO, buffer, n);
	return n;
}

char getchar()
{
	char ch;
	read(STDIN_FILENO, &ch, 1);
	return ch;
}