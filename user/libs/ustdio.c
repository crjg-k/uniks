#include <stdarg.h>
#include <udefs.h>
#include <ulib.h>
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


#define BUFSIZE 1024
static char output_buf[BUFSIZE];

static int skip_atoi(const char **s)
{
	int i = 0;
	while (isdigit(**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}

static char *number(char *str, long num, int base, int size, int precision,
		    int flags)
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
	if ((flags & SIGN) and num < 0) {
		sign = '-';
		num = -num;
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

	if (num == 0) {
		tmp[i++] = '0';
	} else {
		while (num != 0) {
			index = num % base;
			num /= base;
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
	unsigned long num;

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
		if (isdigit(*fmt))
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
			if (isdigit(*fmt))
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
			str = number(str, num, 8, field_width, precision,
				     flags);
			break;
		case 'p':
			flags |= SPECIAL;
			flags |= SMALL;
			if (field_width == -1) {
				field_width = 16;
				flags |= ZEROPAD;
			}
			num = (unsigned long)va_arg(args, void *);
			str = number(str, num, 16, field_width, precision,
				     flags);
			break;
		case 'x':
			flags |= SMALL;
		case 'X':
			num = va_arg(args, unsigned long);
			str = number(str, num, 16, field_width, precision,
				     flags);
			break;
		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			num = va_arg(args, unsigned long);
			str = number(str, num, 10, field_width, precision,
				     flags);
			break;
		case 'n':
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;
		case 'b':   // binary
			num = va_arg(args, unsigned long);
			str = number(str, num, 2, field_width, precision,
				     flags);
			break;
		case 'm':   // mac address
			flags |= (SMALL | ZEROPAD);
			ptr = va_arg(args, char *);
			for (int t = 0; t < 6; t++, ptr++) {
				num = *ptr;
				str = number(str, num, 16, 2, precision, flags);
				*str = ':';
				str++;
			}
			str--;
			break;
		case 'r':   // ip address
			flags |= SMALL;
			ptr = va_arg(args, char *);
			for (int t = 0; t < 4; t++, ptr++) {
				num = *ptr;
				str = number(str, num, 10, field_width,
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

	*str = '\0';

	return str - buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
	int n;
	va_list args;
	va_start(args, fmt);
	n = vsprintf(buf, fmt, args);
	va_end(args);

	return n;
}

int fprintf(int fd, const char *fmt, ...)
{
	int n;
	va_list args;
	va_start(args, fmt);
	n = vsprintf(output_buf, fmt, args);
	va_end(args);

	write(fd, output_buf, n);
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
	int cnt;
	if (empty()) {
		if (ibuffer.front == BUFSIZE)
			ibuffer.front = ibuffer.tail = 0;
		cnt = read(STDIN_FILENO, ibuffer.buf + ibuffer.tail,
			   BUFSIZE - ibuffer.tail);
		if (!cnt)
			return '\0';
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
		if (ch == '\0')
			break;
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
		if (ch == '\0')
			break;
		if (state == 0) {
			s[i++] = ch;
			if (ch != '\n')
				state = 1;
			else
				break;
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
	if (isdigit(ch))
		ch -= '0';
	else if (ch >= 'a' and ch <= 'z')
		ch -= ('a' - 10);
	else
		return -1;
	if (ch >= 0 and ch <= radix - 1)
		return ch;
	else
		return -1;
}

static int getnum(int radix, int *over)
{
	int state = 0, sum = 0;
	while (1) {
		char ch = getchar();
		if (ch == '\0') {
			*over = 1;
			break;
		}
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
	int cnt = 0, state = 0, over = 0;
	char *chp, tmpch;
	int *intp, tmpint;

	while (*fmt) {
		if (state == 0) {
			while (*fmt++ != '%')
				;
			state = '%';
		} else if (state == '%') {
			switch (*fmt) {
			case 'c':
				while (1) {
					if ((tmpch = getchar()) == '\0')
						break;
					if (tmpch != ' ' and tmpch != '\n')
						break;
				}
				if (tmpch == '\0')
					goto ret;
				chp = va_arg(ap, char *);
				*chp = tmpch;
				++cnt;
				break;
			case 's':
				chp = va_arg(ap, char *);
				if (getstr(chp) == 0)
					goto ret;
				++cnt;
				break;
			case 'b':
				tmpint = getnum(2, &over);
				if (over)
					goto ret;
				intp = va_arg(ap, int *);
				*intp = tmpint;
				++cnt;
				break;
			case 'o':
				tmpint = getnum(8, &over);
				if (over)
					goto ret;
				intp = va_arg(ap, int *);
				*intp = tmpint;
				++cnt;
				break;
			case 'd':
				tmpint = getnum(10, &over);
				if (over)
					goto ret;
				intp = va_arg(ap, int *);
				*intp = tmpint;
				++cnt;
				break;
			case 'x':
				tmpint = getnum(16, &over);
				if (over)
					goto ret;
				intp = va_arg(ap, int *);
				*intp = tmpint;
				++cnt;
			}
			++fmt;
			state = 0;
		}
	}

ret:
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