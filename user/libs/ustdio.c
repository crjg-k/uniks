#include <stdarg.h>
#include <udefs.h>
#include <ustdio.h>
#include <ustring.h>
#include <usyscall.h>


// === stdio init ===

void ustdioinit()
{
	ibuffer_init();
}

// === output ===

/**
 * @brief vsprintf.c -- Lars Wirzenius & Linus Torvalds.
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

#define ZEROPAD 0x01
#define SIGN	0x02
#define PLUS	0x04
#define SPACE	0x08
#define LEFT	0x10
#define SPECIAL 0x20
#define SMALL	0x40
#define DOUBLE	0x80

#define is_digit(c) ((c) >= '0' and (c) <= '9')

#define BUFSIZE 1024
static char output_buf[BUFSIZE];

static int skip_atoi(const char **s)
{
	int i = 0;
	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

static char *number(char *str, unsigned int *num, int base, int size,
		    int precision, int flags)
{
	char pad, sign, tmp[36];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i, index;
	char *ptr = str;

	if (flags & SMALL)
		digits = "0123456789abcdefghijklmnopqrstuvwxyz";

	if (flags & LEFT)
		flags &= ~ZEROPAD;

	if (base < 2 or base > 36)
		return 0;

	pad = (flags & ZEROPAD) ? '0' : ' ';

	if (flags & DOUBLE and (*(double *)(num)) < 0) {
		sign = '-';
		*(double *)(num) = -(*(double *)(num));
	} else if (flags & SIGN and !(flags & DOUBLE) and ((int)(*num)) < 0) {
		sign = '-';
		(*num) = -(int)(*num);
	} else
		sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);

	if (sign)
		size--;

	if (flags & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}

	i = 0;

	if (flags & DOUBLE) {
		unsigned int ival = (unsigned int)(*(double *)num);
		unsigned int fval =
			(unsigned int)(((*(double *)num) - ival) * 1000000);
		do {
			index = (fval) % base;
			(fval) /= base;
			tmp[i++] = digits[index];
		} while (fval);
		tmp[i++] = '.';

		do {
			index = (ival) % base;
			(ival) /= base;
			tmp[i++] = digits[index];
		} while (ival);
	} else if ((*num) == 0) {
		tmp[i++] = '0';
	} else {
		while ((*num) != 0) {
			index = (*num) % base;
			(*num) /= base;
			tmp[i++] = digits[index];
		}
	}

	if (i > precision)
		precision = i;

	size -= precision;

	if (!(flags & (ZEROPAD + LEFT)))
		while (size-- > 0)
			*str++ = ' ';

	if (sign)
		*str++ = sign;

	if (flags & SPECIAL) {
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if (!(flags & LEFT))
		while (size-- > 0)
			*str++ = pad;

	while (i < precision--)
		*str++ = '0';

	while (i-- > 0)
		*str++ = tmp[i];

	while (size-- > 0)
		*str++ = ' ';
	return str;
}

static int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len, i;

	char *str, *s, *ptr;
	int *ip;

	int flags, field_width, precision, qualifier;
	unsigned int num;

	for (str = buf; str - buf < BUFSIZE and *fmt; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}

		flags = 0;
repeat:
		++fmt;
		switch (*fmt) {
		case '-':
			flags |= LEFT;
			goto repeat;
		case '+':
			flags |= PLUS;
			goto repeat;
		case ' ':
			flags |= SPACE;
			goto repeat;
		case '#':
			flags |= SPECIAL;
			goto repeat;
		case '0':
			flags |= ZEROPAD;
			goto repeat;
		}

		field_width = -1;
		if (is_digit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		precision = -1;
		if (*fmt == '.') {
			++fmt;
			if (is_digit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		qualifier = -1;
		if (*fmt == 'h' or *fmt == 'l' or *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char)va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;
		case 's':
			s = va_arg(args, char *);
			len = strlen(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;
			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			break;
		case 'o':
			num = va_arg(args, unsigned long);
			str = number(str, &num, 8, field_width, precision,
				     flags);
			break;
		case 'p':
			if (field_width == -1) {
				field_width = 8;
				flags |= ZEROPAD;
			}
			num = va_arg(args, unsigned long);
			str = number(str, &num, 16, field_width, precision,
				     flags);
			break;
		case 'x':
			flags |= SMALL;
		case 'X':
			num = va_arg(args, unsigned long);
			str = number(str, &num, 16, field_width, precision,
				     flags);
			break;
		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			num = va_arg(args, unsigned long);
			str = number(str, &num, 10, field_width, precision,
				     flags);
			break;
		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;
		case 'f':
			flags |= SIGN;
			flags |= DOUBLE;
			double dnum = va_arg(args, double);
			str = number(str, (unsigned int *)&dnum, 10,
				     field_width, precision, flags);
			break;
		case 'b':   // binary
			num = va_arg(args, unsigned long);
			str = number(str, &num, 2, field_width, precision,
				     flags);
			break;
		case 'm':   // mac address
			flags |= SMALL | ZEROPAD;
			ptr = va_arg(args, char *);
			for (unsigned long t = 0; t < 6; t++, ptr++) {
				int num = *ptr;
				str = number(str, &num, 16, 2, precision,
					     flags);
				*str = ':';
				str++;
			}
			str--;
			break;
		case 'r':   // ip address
			flags |= SMALL;
			ptr = va_arg(args, char *);
			for (unsigned long t = 0; t < 4; t++, ptr++) {
				int num = *ptr;
				str = number(str, &num, 10, field_width,
					     precision, flags);
				*str = '.';
				str++;
			}
			str--;
			break;
		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}

	return str - buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int n = vsprintf(buf, fmt, args);
	va_end(args);
	return n;
}

int printf(const char *fmt, ...)
{
	int n;
	va_list args;
	va_start(args, fmt);
	n = vsprintf(output_buf, fmt, args);
	va_end(args);

	write(STDOUT_FILENO, output_buf, n);
	return n;
}

// === input ===

struct {
	char buf[BUFSIZE];
	int front, tail;
} ibuffer;

static void ibuffer_init()
{
	ibuffer.front = ibuffer.tail = 0;
}

static int empty()
{
	return ibuffer.front == ibuffer.tail;
}

char getchar()
{
	if (empty()) {
		if (ibuffer.front == BUFSIZE)
			ibuffer.front = ibuffer.tail = 0;
		int cnt = read(STDIN_FILENO, ibuffer.buf + ibuffer.tail,
			       BUFSIZE - ibuffer.tail);
		ibuffer.tail += cnt;
	}
	return ibuffer.buf[ibuffer.front++];
}

static char backchar()
{
	--ibuffer.front;
}

static int getstr(char *s)
{
	int state = 0, i = 0;
	while (1) {
		char ch = getchar();
		if (state == 0) {
			if (ch != ' ' and ch != '\n') {
				state = 1;
				s[i++] = ch;
			}
		} else if (state == 1) {
			if (ch == ' ' or ch == '\n')
				break;
			s[i++] = ch;
		}
	}
	s[i] = '\0';
	return i;
}

int getline(char *s, int n)
{
	int state = 0, i = 0;
	while (i < n) {
		char ch = getchar();
		if (state == 0) {
			if (ch != '\n') {
				state = 1;
				s[i++] = ch;
			}
		} else if (state == 1) {
			s[i++] = ch;
			if (ch == '\n')
				break;
		}
	}
	s[i] = '\0';
	return i;
}

static char matchchar(int radix, char ch)
{
	if (is_digit(ch))
		ch -= '0';
	else if (ch >= 'a' and ch <= 'f')
		ch -= ('a' - 10);
	else
		return -1;
	if (ch >= 0 and ch <= radix - 1)
		return ch;
	else
		return -1;
}

static int getnum(int radix)
{
	int state = 0, sum = 0;
	while (1) {
		char ch = getchar();
		if (state == 0) {
			if (ch != ' ' and ch != '\n') {
				state = 1;
				goto handle;
			}
		} else if (state == 1) {
handle:
			if (ch == '\n' or ch == ' ' or
			    (ch = matchchar(radix, ch)) == -1) {
				backchar();
				break;
			}
			sum *= radix;
			sum += ch;
		}
	}
	return sum;
}

static int vscanf(const char *fmt, va_list ap)
{
	int cnt = 0, state = 0;
	char *chp;
	int *intp;

	while (*fmt) {
		if (state == 0) {
			while (*fmt++ != '%')
				;
			state = '%';
		} else if (state == '%') {
			++cnt;
			switch (*fmt) {
			case 'c':
				chp = va_arg(ap, char *);
				while (1) {
					*chp = getchar();
					if (*chp != ' ' and *chp != '\n')
						break;
				}
				break;
			case 's':
				chp = va_arg(ap, char *);
				getstr(chp);
				break;
			case 'b':
				intp = va_arg(ap, int *);
				*intp = getnum(2);
				break;
			case 'o':
				intp = va_arg(ap, int *);
				*intp = getnum(8);
				break;
			case 'd':
				intp = va_arg(ap, int *);
				*intp = getnum(10);
				break;
			case 'x':
				intp = va_arg(ap, int *);
				*intp = getnum(16);
				break;
			default:
				--cnt;
			}
			++fmt;
			state = 0;
		}
	}
	return cnt;
}

int scanf(const char *fmt, ...)
{
	int n;
	va_list ap;
	va_start(ap, fmt);
	n = vscanf(fmt, ap);
	va_end(ap);
	return n;
}