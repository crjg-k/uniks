/**
 * @file kstdio.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief 
 * @version 0.1
 * @date 2023-01-29 18:00:08
 * 
 * @copyright Copyright (c) 2023 crjg-k
 * 
 */

#include <console.h>
#include <kstdio.h>

/* HIGH level console I/O */

static char digits[] = "0123456789ABCDEF";

char getchar()
{
	return (char)console_getchar();
}

void kputc(char c)
{
	console_putchar(c);
}
void kputs(const char *str)
{
	char c;
	while ((c = *str++) != '\0')
		kputc(c);
}

static void printint(int32_t xx, int32_t base, int32_t sgn)
{
	char buf[16];
	int32_t i, neg;
	uint32_t x;

	neg = 0;
	if (sgn && xx < 0) {
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
static void printptr(uint64_t x)
{
	kputs("0x");
	for (int32_t i = 0; i < (sizeof(uint64_t) * 2); i++, x <<= 4)
		kputc(digits[x >> (sizeof(uint64_t) * 8 - 4)]);
}
static void vkprintf(const char *fmt, va_list ap)
{
	char *s;
	int32_t c, i, state;

	state = 0;
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
				printint(va_arg(ap, int32_t), 10, 1);
			} else if (c == 'l') {
				printint(va_arg(ap, uint64_t), 10, 0);
			} else if (c == 'x') {
				printint(va_arg(ap, int32_t), 16, 0);
			} else if (c == 'p') {
				printptr(va_arg(ap, uint64_t));
			} else if (c == 's') {
				s = va_arg(ap, char *);
				if (s == 0)
					s = "(null)";
				while (*s != 0) {
					kputc(*s);
					s++;
				}
			} else if (c == 'c') {
				kputc(va_arg(ap, uint32_t));
			} else if (c == '%') {
				kputc(c);
			} else {
				// Unknown % sequence.  Print it to draw attention.
				kputc('%');
				kputc(c);
			}
			state = 0;
		}
	}
}

void kprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vkprintf(fmt, ap);
	va_end(ap);
}