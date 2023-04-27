#include "tty.h"
#include "device.h"
#include <process/proc.h>


int32_t current_tty = -1;

// just exist 3 tty devices temporarily, since Linux-0.11 do so
struct tty_struct_t tty_table[NTTY] = {
	{
		{},
		&uart_struct,
		tty_read,
		tty_write,
		do_tty_interrupt,
		{},
	},
	{
		{},
		NULL,
		tty_read,
		tty_write,
		do_tty_interrupt,
		{},
	},
	{
		{},
		NULL,
		tty_read,
		tty_write,
		do_tty_interrupt,
		{},
	},
};

void tty_init()
{
	char name[5] = "tty ";
	dev_t dev;
	struct device_t *devptr;
	for (int32_t i = NTTY - 1; i >= 0; i--) {
		initlock(&tty_table[i].secondary_q.tty_queue_lock,
			 "tty_queue_lock");
		queue_init(&tty_table[i].secondary_q.qm, TTY_BUF_SIZE,
			   tty_table[i].secondary_q.tty_buf_array);
		INIT_LIST_HEAD(&tty_table[i].secondary_q.wait_list);
		devptr = get_null_device();
		name[3] = '0' + i;
		dev = device_install(DEV_CHAR, DEV_TTY, &tty_table[i], name, 0,
				     tty_table[i].tty_interrupt_handler, NULL,
				     tty_table[i].read, tty_table[i].write,
				     devptr);
	}
	current_tty = dev;
}

/**
 * @brief copy content of uart_rx_queue to tty->secondary_q but without any
 * termios process since that there is no implementation of it temporarily
 * @param tty
 */
void copy_to_cooked(struct tty_struct_t *tty)
{
	char ch;

	acquire(&tty->secondary_q.tty_queue_lock);
	while ((uart_read(tty->uart_associated, &ch, 1)) != 0) {
		if (queue_full(&tty->secondary_q
					.qm))	// discard the oldest simply
			queue_pop(&tty->secondary_q.qm);
		queue_push_chartype(&tty->secondary_q.qm, ch);
		// echo mode
		uart_write(tty->uart_associated, &ch, 1);
	}

	proc_unblock_all(&tty->secondary_q.wait_list);
	release(&tty->secondary_q.tty_queue_lock);
}

/**
 * @brief processing recieved chars according to POSIX.termios
 * standards, and then push into tty's secondary queue
 */
void do_tty_interrupt(void *ttyptr)
{
	copy_to_cooked((struct tty_struct_t *)ttyptr);
}

/**
 * @brief read until encounter a newline('\n' or '\r') or up to cnt
 *
 * @param ttyptr
 * @param buf
 * @param cnt
 * @return int32_t: read bytes count, guarantee that at least 1
 */
int32_t tty_read(void *ttyptr, void *buf, size_t cnt)
{
	int32_t n = 0;
	char *buffer = (char *)buf, ch;
	struct tty_struct_t *tty = ttyptr;

	acquire(&tty->secondary_q.tty_queue_lock);
	while (n < cnt) {
		while (queue_empty(&tty->secondary_q.qm)) {
			if (n == 0) {
				proc_block(myproc(),
					   &tty->secondary_q.wait_list,
					   TASK_BLOCK);
				release(&tty->secondary_q.tty_queue_lock);
				sched();
				acquire(&tty->secondary_q.tty_queue_lock);
			} else
				goto over;
		}
		ch = *(char *)queue_front_chartype(&tty->secondary_q.qm);
		queue_pop(&tty->secondary_q.qm);
		if (ch == EOF)
			goto over;
		*(buffer++) = ch;
		n++;
		if (ch == '\n' or ch == '\r')
			goto over;
	}
over:
	release(&tty->secondary_q.tty_queue_lock);

	return n;
}

int32_t tty_write(void *ttyptr, void *buf, size_t cnt)
{
	struct tty_struct_t *tty = ttyptr;
	return uart_write(tty->uart_associated, buf, cnt);
}
