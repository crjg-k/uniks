#include "tty.h"
#include "console.h"
#include "uart.h"


// just exist 3 tty devices temporarily, since Linux-0.11 do so
struct tty_struct_t tty_table[NTTY] = {
	{
		{},
		0,
		console_read,
		console_write,
		{},
	},
	{
		{},
		0,
		console_read,
		console_write,
		{},
	},
	{
		{},
		0,
		console_read,
		console_write,
		{},
	},
};

void tty_init()
{
	for (int32_t i = 0; i < NTTY; i++) {
		initlock(&tty_table[i].secondary_q.tty_queue_lock,
			 "tty_queue_lock");
		queue_init(&tty_table[i].secondary_q.qm, TTY_BUF_SIZE,
			   tty_table[i].secondary_q.tty_buf_array);
	}
}

void do_tty_interrupt(int32_t tty)
{
	copy_to_cooked(tty_table + tty);
}

/**
 * @brief copy content of uart_rx_queue to tty->secondary_q but without any
 * termios process since that there is no implement of it temporarily
 * @param tty
 */
void copy_to_cooked(struct tty_struct_t *tty)
{
	acquire(&uart_rx_queue.uart_rxbuf_lock);
	acquire(&tty->secondary_q.tty_queue_lock);

	while (!queue_empty(&uart_rx_queue.qm)) {
		if (queue_full(&tty->secondary_q
					.qm))	// discard the oldest simply
			queue_pop(&tty->secondary_q.qm);
		queue_push_chartype(&tty->secondary_q.qm,
				    *(char *)queue_front(&uart_rx_queue.qm));
		queue_pop(&uart_rx_queue.qm);
	}

	release(&tty->secondary_q.tty_queue_lock);
	release(&uart_rx_queue.uart_rxbuf_lock);
}