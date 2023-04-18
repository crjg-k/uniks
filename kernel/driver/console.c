/**
 * @file console.c
 * @author crjg-k (crjg-k@qq.com)
 * @brief
 * @version 0.1
 * @date 2023-01-29 16:51:46
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */

#include "console.h"
#include "uart.h"
#include"tty.h"
#include <platform/sbi.h>


void consoleinit()
{
	uartinit();
	tty_init();
}

int64_t console_putchar(char c)
{
	return sbi_console_putchar(c);
}

int64_t console_getchar()
{
	return sbi_console_getchar();
}

int32_t console_read()
{
	kprintf("111\n");
	return 1;
}

int32_t console_write()
{
	kprintf("222\n");
	return 1;
}