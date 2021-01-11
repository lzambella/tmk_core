/**
 * UART functions with flow control
 * 
 * 2021 Luke Zambella
 */

#include <avr/interrupt.h>
#include <avr/io.h>

#define BUFFER_LEN 256
/**
 * Initialize the UART service
 */
void uart_service_init(uint8_t baud_rate);
/**
 * Append a single byte of data to the tramsmit buffer
 */
void uart_xmit(char data);
/**
 * Append a string of bytes to the transmit buffer
 */
void uart_xmit_str(char * data, uint8_t size);
/**
 * Receive a single byte
 */
uint8_t uart_rceiv();
/**
 * Sends the contents of the buffer one at a time
 * This function should be ran in an event loop constantly
 */
void send_xmit_buf();
/**
 * Enable the CTS line for receviving data from bluefruit
 */
void uart_cts_enable();
/**
 * Disable the CTS line for receiving data from bluefruit
 */
void uart_cts_disable();

void uart_receive();