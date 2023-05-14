#ifndef __KERNEL_DEVICE_TTY_H__
#define __KERNEL_DEVICE_TTY_H__

#include "device.h"
#include "uart.h"
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>
#include <uniks/queue.h>
#include <uniks/termios.h>


#define NTTY (3)

struct tty_queue_t {
	struct spinlock_t tty_queue_lock;
	struct list_node_t wait_list;	// blocked process which waits for input
	char tty_buf_array[TTY_BUF_SIZE];
	struct queue_meta_t qm;
};

struct tty_struct_t {
	// data struct of terminal I/O attribute and control characters
	struct termios_t termios;
	struct uart_struct_t *uart_associated;
	int32_t (*read)(void *ttyptr, void *buf, size_t cnt);
	int32_t (*write)(void *ttyptr, void *buf, size_t cnt);
	void (*tty_interrupt_handler)(void *ttyptr);
	// tty secondary queue (after process with termios standard) for read
	struct tty_queue_t secondary_q;
};

extern struct tty_struct_t tty_table[];	  // tty struct array
extern int32_t current_tty;


void tty_init();
void copy_to_cooked(struct tty_struct_t *tty);
void do_tty_interrupt(void *ttyptr);
int32_t tty_read(void *ttyptr, void *buf, size_t cnt);
int32_t tty_write(void *ttyptr, void *buf, size_t cnt);


#endif /* !__KERNEL_DEVICE_TTY_H__ */