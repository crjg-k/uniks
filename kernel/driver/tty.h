#ifndef __KERNEL_DRIVER_TTY_H__
#define __KERNEL_DRIVER_TTY_H__

#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>
#include <uniks/param.h>
#include <uniks/queue.h>
#include <uniks/termios.h>


#define NTTY 3

struct tty_queue_t {
	struct spinlock_t tty_queue_lock;
	struct list_node_t wait_list;	// blocked process which waits for input
	char tty_buf_array[TTY_BUF_SIZE];
	struct queue_meta_t qm;
};

struct tty_struct_t {
	// data struct of terminal I/O attribute and control characters
	struct termios_t termios;
	int32_t pgrp;	      // process group it belongs to
	int32_t (*read)();    // oop but C version, interface thought
	int32_t (*write)();   // oop but C version, interface thought
	// tty secondary queue (after process with termios standard) for read
	struct tty_queue_t secondary_q;
};

extern struct tty_struct_t tty_table[];	  // tty struct array


void tty_init();
void do_tty_interrupt(int32_t tty);
void copy_to_cooked(struct tty_struct_t *tty);


#endif /* !__KERNEL_DRIVER_TTY_H__ */