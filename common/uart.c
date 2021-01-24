/**
 * UART functions with flow control
 * 
 * 2021 Luke Zambella
 */
#include "uart.h"
#include <debug.h>
#include <avr/delay.h>
// transmit buffer and index
// idx_tail is always empty
char xmit_buf[BUFFER_LEN];

// receive buffer
char receiv_buf[BUFFER_LEN];

uint8_t xmit_idx_head = 0;
uint8_t xmit_idx_tail = 0;

// interrupt index for transferring data
uint8_t xmit_ser_idx = 0;


#define USART_BAUDRATE 115200
#define BAUD_PRESCALE ((( F_CPU / ( USART_BAUDRATE * 8UL))) - 1)
void uart_service_init() {
    // CTS is an INTPUT on the bluefruit but OUT on the teensy
    // setting CTS to LOW allows data to be transferred out from the BLUEFRUIT
    DDRD |= (1 << 5);   // CTS pin D5 set as OUTPUT
    PORTD |= (1 << 5);  // set HIGH by default

    // RTS is an OUT on the BLUEFRUIT 
    // it is an INPUT on the MCU
    // RTS from the bluefruit tells the MCU that data can be sent INTO the bluefruit
    DDRB &= ~(1 << 7);   // RTS input (low)

    unsigned int baud = BAUD_PRESCALE;
    
    UBRR1H = (unsigned char) (baud >> 8 );
    UBRR1L = (unsigned char)baud;
    
    // enable received and transmitter
    //UCSR1B = ( 1 << RXEN1 ) | ( 1 << TXEN1 );
    // Set to double speed mode
    UCSR1A |= (1 << U2X1);
    // Enable transfer only
    UCSR1B = ( 1 << TXEN1 );

    UCSR1C = ( 1 << USBS1 ) | ( 3 << UCSZ10 );

    //UCSR1D |= (0<<RTSEN) | (0<<CTSEN);  // RTS, CTS
}

void uart_xmit(char data) {
    // Buffer full, do nothing
    // This should never happen
    if (xmit_idx_tail == BUFFER_LEN) {
        return;
    }

    xmit_buf[xmit_idx_tail] = data;
    xmit_idx_tail++;
}
void uart_xmit_str(char * data, uint8_t size) {
    for(uint8_t i = 0; i < size; i++) {
        uart_xmit(data[i]);
    }
}
void uart_receive() {
    // Data comes form RX buffer
    // if no data present then return
    if ( UCSR1A & ( 1 << RXC1) ) {
        // Print the hex data
        while (!( UCSR1A & ( 1 << RXC1) ));
        xprintf("%c", UDR1);
    }
}
void send_xmit_buf() {
    // If RTS is high then we cant send data
    if ((PINB & (1 << 7)) >> 7 == 1) {
        //print("Not clear to send!");
        //return;
    }

    // return if no data in the buffer
    if (xmit_idx_head == 0 && xmit_idx_tail == 0) {
        return;
    }
    // enable the cts line to send data to bluefruit
    uart_cts_enable();

    // Return if the send buffer is not ready
    // xmit_idx_head ensures that the correct bit will be sent the 
    // next time buffer is free
    if (( UCSR1A & (1 << UDRE1) ) == 0) {
        return;
    }

    // Send byte and increment the head for the next byte
    UDR1 = xmit_buf[xmit_idx_head];
    xmit_idx_head++;
    if (xmit_idx_head == xmit_idx_tail) {
        // Reset buffer index
        xmit_idx_tail = 0;
        xmit_idx_head = 0;
    }
    // CTS is input on bluefruit, output on teensy
    // tell bluefruit that we are finished sending data by diabling pin
    uart_cts_disable();
}

void uart_cts_enable() {
    PORTD |= (1 << 5);
}

void uart_cts_disable() {
    PORTD &= ~(1 << 5);
}