#include "tty.h"
#include "device.h"
#include <mm/phys.h>
#include <process/proc.h>


int32_t current_tty = -1;

// just exist 3 tty devices temporarily, since Linux-0.11 do so
struct tty_struct_t tty_table[NTTY] = {
	{
		.uart_associated = &uart_struct,
		.read = tty_read,
		.write = tty_write,
		.tty_interrupt_handler = do_tty_interrupt,
		.secondary_q = {},
	},
	{
		.uart_associated = NULL,
		.read = tty_read,
		.write = tty_write,
		.tty_interrupt_handler = do_tty_interrupt,
		.secondary_q = {},
	},
	{
		.uart_associated = NULL,
		.read = tty_read,
		.write = tty_write,
		.tty_interrupt_handler = do_tty_interrupt,
		.secondary_q = {},
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
	while ((uart_read(tty->uart_associated, 0, &ch, 1)) != 0) {
		ch = (ch == '\r') ? '\n' : ch;
		if (ch == BACKSPACE) {
			if (!queue_empty(&tty->secondary_q.qm)) {
				queue_back_pop(&tty->secondary_q.qm);
			}
		} else {
			if (queue_full(&tty->secondary_q.qm)) {
				// discard the oldest character
				queue_front_pop(&tty->secondary_q.qm);
			}
			assert(!queue_full(&tty->secondary_q.qm));
			queue_push_chartype(&tty->secondary_q.qm, ch);
		}

		// echo mode
		{
			if (ch == BACKSPACE)
				uart_write(tty->uart_associated, 0, "\b \b", 3);
			else
				uart_write(tty->uart_associated, 0, &ch, 1);
		}
	}
	if (ch == '\n' or ch == EOF)
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
 * @param ttyptr
 * @param user_dst
 * @param buf
 * @param cnt
 * @return int64_t: read bytes count, guarantee that at least 1
 */
int64_t tty_read(void *ttyptr, int32_t user_dst, void *buf, size_t cnt)
{
	assert(cnt <= LINE_MAXN);
	int64_t n = 0;
	char *buffer = kmalloc(cnt), ch;
	struct tty_struct_t *tty = ttyptr;

	acquire(&tty->secondary_q.tty_queue_lock);
	while (n < cnt) {
		while (queue_empty(&tty->secondary_q.qm)) {
			proc_block(&tty->secondary_q.wait_list,
				   &tty->secondary_q.tty_queue_lock);
		}
		assert(!queue_empty(&tty->secondary_q.qm));
		ch = *(char *)queue_front_chartype(&tty->secondary_q.qm);
		queue_front_pop(&tty->secondary_q.qm);
		buffer[n++] = ch;
		if (ch == '\n' or ch == EOF)
			break;
	}

	assert(n >= 1);
	assert(either_copyout(user_dst, buf, buffer, n) != -1);
	kfree(buffer);
	release(&tty->secondary_q.tty_queue_lock);

	return n;
}

int64_t tty_write(void *ttyptr, int32_t user_src, void *buf, size_t cnt)
{
	assert(cnt <= LINE_MAXN);
	char *buffer = kmalloc(cnt);
	assert(either_copyin(user_src, buffer, buf, cnt) != -1);
	struct tty_struct_t *tty = ttyptr;
	int64_t n = uart_write(tty->uart_associated, 0, buffer, cnt);
	kfree(buffer);
	return n;
}
