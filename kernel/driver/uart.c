#include "uart.h"
#include <uniks/queue.h>


// note: the transmit output buffer equals to tty write buffer (in tty.c)

extern void do_tty_interrupt(int32_t tty);

// UART recieve queue (buffer) entity
struct uart_rx_queue_t uart_rx_queue;
int32_t current_tty = 0;

void uartrecieve_queue_init()
{
	initlock(&uart_rx_queue.uart_rxbuf_lock, "uart_rxbuf_lock");
	queue_init(&uart_rx_queue.qm, UART_TX_BUF_SIZE,
		   uart_rx_queue.uart_rx_buf_array);
}

void uartinit()
{
	// disable UART interrupts
	UART_WRITEREG(IER, 0x00);

	// special mode to set baud rate
	UART_WRITEREG(LCR, UART_READREG(LCR) | LCR_BAUD_LATCH);

	// LSB for baud rate of 38.4K
	UART_WRITEREG(DLL, 0x03);

	// MSB for baud rate of 38.4K
	UART_WRITEREG(DLM, 0x00);

	// leave set-baud mode, and set word length to 8 bits, no parity
	UART_WRITEREG(LCR, 0 | LCR_EIGHT_BITS);

	// reset and enable FIFOs.
	UART_WRITEREG(FCR,
		      UART_READREG(FCR) | FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

	// enable receive interrupt but without transmit interrupt.
	UART_WRITEREG(IER, UART_READREG(IER) | IER_RX_ENABLE);

	uartrecieve_queue_init();
}

// read one input char from UART. return EOF if none is waiting
__always_inline char uartgetc()
{
	if (UART_READREG(LSR) & LSR_RX_READY) {
		// input data is ready
		return UART_READREG(RHR);
	} else {
		return EOF;
	}
}

/**
 * @brief handle a uart interrupt, raised because input has arrived, or the uart
 * is ready for more output, or both. called from external_interrupt_handler().
 */
void uartinterrupt_handler()
{
	// read and process incoming chars
	acquire(&uart_rx_queue.uart_rxbuf_lock);
	while (1) {
		char c = uartgetc();
		if (c == EOF)
			break;
		kprintf("%c", c);
		if (queue_full(
			    &uart_rx_queue.qm))	  // discard the oldest simply
			queue_pop(&uart_rx_queue.qm);
		queue_push_chartype(&uart_rx_queue.qm, c);
	}
	release(&uart_rx_queue.uart_rxbuf_lock);
	/**
	 * @brief processing recieved chars according to POSIX.termios
	 * standards, and then push into tty's secondary queue
	 */
	do_tty_interrupt(current_tty);
}
