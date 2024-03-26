#include "uart.h"
#include "tty.h"
#include <platform/sbi.h>
#include <uniks/kassert.h>
#include <uniks/queue.h>


// note: the transmit output buffer equals to tty write buffer (in tty.c)


// UART entity
struct uart_struct_t uart_struct;

__always_inline void uart_queue_init()
{
	initlock(&uart_struct.uart_rx_queue.uart_rxbuf_lock, "uart_rxbuf_lock");
	initlock(&uart_struct.uart_tx_queue.uart_txbuf_lock, "uart_txbuf_lock");
	queue_init(&uart_struct.uart_rx_queue.qm, UART_TX_BUF_SIZE,
		   uart_struct.uart_rx_queue.uart_rx_buf_array);
}

__always_inline void uart_hard_init()
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
}

__always_inline void uart_soft_init()
{
	uart_queue_init();
	char uart0_name[] = "uart0";
	device_install(DEV_CHAR, DEV_SERIAL, &uart_struct, uart0_name,
		       current_tty, do_uart_interrupt, NULL, uart_read,
		       uart_write, &devices[UART0_IRQ]);
}

void uart_init()
{
	uart_hard_init();
	uart_soft_init();
}

/**
 * @brief handle a uart interrupt, raised because input has arrived, or the uart
 * is ready for more output, or both. called from external_interrupt_handler().
 * @param uartptr
 */
void do_uart_interrupt(void *uartptr)
{
	struct uart_struct_t *uart = uartptr;
	// read and process incoming chars
	acquire(&uart->uart_rx_queue.uart_rxbuf_lock);
	while (1) {
		char c = uart_getchar();
		if (c == EOF)
			break;
		if (queue_full(&uart->uart_rx_queue
					.qm))	// discard the oldest simply
			queue_front_pop(&uart->uart_rx_queue.qm);
		queue_push_chartype(&uart->uart_rx_queue.qm, c);
	}
	release(&uart->uart_rx_queue.uart_rxbuf_lock);
}

// read one input char from UART. return EOF if none is waiting
__always_inline char uart_getchar()
{
	if (get_var_bit(UART_READREG(LSR), LSR_RX_READY)) {
		// input data is ready
		return UART_READREG(RHR);
	} else {
		return EOF;
	}
}

/**
 * @brief read and return immediately if the uart->uart_rx_queue is empty
 * @param uartptr
 * @param user_dst
 * @param buf
 * @param cnt
 * @return int64_t: read bytes count, don't guarantee up to cnt
 */
int64_t uart_read(void *uartptr, int32_t user_dst, void *buf, size_t cnt)
{
	int64_t n = 0;
	char *buffer = buf;
	struct uart_struct_t *uart = uartptr;
	assert(user_dst == 0);

	acquire(&uart->uart_rx_queue.uart_rxbuf_lock);
	while (n < cnt) {
		if (queue_empty(&uart->uart_rx_queue.qm))
			goto over;
		*(buffer++) =
			*(char *)queue_front_chartype(&uart->uart_rx_queue.qm);
		queue_front_pop(&uart->uart_rx_queue.qm);
		n++;
	}
over:
	release(&uart->uart_rx_queue.uart_rxbuf_lock);

	return n;
}

/**
 * @brief write through to uart physical device without any buffer array
 * @param uartptr
 * @param user_src
 * @param buf
 * @param cnt
 * @return int64_t: write bytes count
 */
int64_t uart_write(void *uartptr, int32_t user_src, void *buf, size_t cnt)
{
	int64_t n = 0;
	char *buffer = buf;
	struct uart_struct_t *uart = uartptr;
	assert(user_src == 0);

	acquire(&uart->uart_tx_queue.uart_txbuf_lock);
	while (cnt-- > 0) {
		sbi_console_putchar(*(buffer++));
		n++;
	}
	release(&uart->uart_tx_queue.uart_txbuf_lock);

	return n;
}
