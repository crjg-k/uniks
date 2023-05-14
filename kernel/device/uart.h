/**
 * @file uart.h
 * @author crjg-k (crjg-k@qq.com)
 * @brief low-level driver routines for 16550a UART.
 * @version 0.1
 * @date 2023-04-17 10:10:48
 *
 * @copyright Copyright (c) 2023 crjg-k
 *
 */
#ifndef __KERNEL_DEVICE_UART_H__
#define __KERNEL_DEVICE_UART_H__


#include <platform/platform.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/param.h>
#include <uniks/queue.h>


/**
 * @brief the UART control registers are memory-mapped at address UART0. this
 * macro returns the address of one of the registers.
 */
#define UART_REG(reg)	((volatile uint8_t *)(UART0 + reg))
/**
 * @brief the UART control registers. some have different meanings for read vs
 * write. More information see http://byterunner.com/16550.html
 */
#define RHR		(0)   // Receive Holding Register (read mode)
#define THR		(0)   // Transmit Holding Register (write mode)
#define DLL		(0)   // LSB of Divisor Latch (write mode)
#define IER		(1)   // Interrupt Enable Register (write mode)
#define DLM		(1)   // MSB of Divisor Latch (write mode)
#define FCR		(2)   // FIFO Control Register (write mode)
#define ISR		(2)   // Interrupt Status Register (read mode)
#define LCR		(3)   // Line Control Register
#define MCR		(4)   // Modem Control Register
#define LSR		(5)   // Line Status Register
#define MSR		(6)   // Modem Status Register
#define SPR		(7)   // ScratchPad Registers
#define IER_RX_ENABLE	(1 << 0)
#define IER_TX_ENABLE	(1 << 1)
#define FCR_FIFO_ENABLE (1 << 0)
#define FCR_FIFO_CLEAR	(3 << 1)   // clear the content of the two FIFOs
#define LCR_EIGHT_BITS	(3 << 0)
#define LCR_BAUD_LATCH	(1 << 7)   // special mode to set baud rate
#define LSR_RX_READY	(1 << 0)   // input is waiting to be read from RHR
#define LSR_TX_IDLE	(1 << 5)   // THR can accept another character to send

#define UART_READREG(reg)	(*(UART_REG(reg)))
#define UART_WRITEREG(reg, val) (*(UART_REG(reg)) = (val))


// UART recieve queue (buffer)
struct uart_rx_queue_t {
	struct spinlock_t uart_rxbuf_lock;
	char uart_rx_buf_array[UART_TX_BUF_SIZE];
	struct queue_meta_t qm;
};

// UART transmit queue (no buffer and write through)
struct uart_tx_queue_t {
	struct spinlock_t uart_txbuf_lock;
};

// UART structure contain transmit queue and recieve queue
struct uart_struct_t {
	struct uart_rx_queue_t uart_rx_queue;
	struct uart_tx_queue_t uart_tx_queue;
};

extern struct uart_struct_t uart_struct;

void uart_init();
char uart_getchar();
int32_t uart_read(void *uartptr, void *buf, size_t cnt);
int32_t uart_write(void *uartptr, void *buf, size_t cnt);
void do_uart_interrupt(void *uartptr);


#endif /* !__KERNEL_DEVICE_UART_H__ */