/* USB Keyboard Plus Debug Channel Example for Teensy USB Development Board
 * http://www.pjrc.com/teensy/usb_keyboard.html
 * Copyright (c) 2009 PJRC.COM, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "keycode.h"
#include "usb_keyboard.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "host.h"
#include <stdio.h>
#include <uart.h>
#include "config.h"
// protocol setting from the host.  We use exactly the same report
// either way, so this variable only stores the setting since we
// are required to be able to report which setting is in use.
uint8_t keyboard_protocol=1;

// the idle configuration, how often we send the report to the
// host (ms * 4) even when it hasn't changed
// Windows and Linux set 0 while OS X sets 6(24ms) by SET_IDLE request.
uint8_t keyboard_idle=125;

// count until idle timeout
uint8_t usb_keyboard_idle_count=0;

// 1=num lock, 2=caps lock, 4=scroll lock, 8=compose, 16=kana
volatile uint8_t usb_keyboard_leds=0;
// CMD mode code for bluefruit
uint8_t gatt_cmd[] = "AT+BLEKEYBOARDCODE=";

static inline int8_t send_report(report_keyboard_t *report, uint8_t endpoint, uint8_t keys_start, uint8_t keys_end);
static inline int8_t send_report_ble(report_keyboard_t *report, uint8_t endpoint, uint8_t keys_start, uint8_t keys_end);
static inline void uart_putc(uint8_t dat);

int8_t usb_keyboard_send_report(report_keyboard_t *report)
{
    int8_t result = 0;

#ifdef NKRO_ENABLE
    if (keyboard_nkro)
        result = send_report(report, KBD2_ENDPOINT, 0, KBD2_SIZE);
    else
#endif
    {
        // If bluetooth is enabled for use then send a report via that instead
        #ifdef BLUETOOTH_ENABLE
        result = send_report_ble(report, KBD_ENDPOINT, 0, KBD_SIZE);
        #else
        result = send_report(report, KBD_ENDPOINT, 0, KBD_SIZE);
        #endif
    }
    if (result) return result;
    usb_keyboard_idle_count = 0;
    usb_keyboard_print_report(report);
    return 0;
}

void usb_keyboard_print_report(report_keyboard_t *report)
{
    if (!debug_keyboard) return;
    print("keys: ");
    for (int i = 0; i < KEYBOARD_REPORT_KEYS; i++) { phex(report->keys[i]); print(" "); }
    print(" mods: "); phex(report->mods); print("\n");
}


static inline int8_t send_report(report_keyboard_t *report, uint8_t endpoint, uint8_t keys_start, uint8_t keys_end)
{
    uint8_t intr_state, timeout;

    if (!usb_configured()) return -1;
    intr_state = SREG;
    cli();
    UENUM = endpoint;
    timeout = UDFNUML + 50;
    while (1) {
            // are we ready to transmit?
            if (UEINTX & (1<<RWAL)) break;
            SREG = intr_state;
            // has the USB gone offline?
            if (!usb_configured()) return -1;
            // have we waited too long?
            if (UDFNUML == timeout) return -1;
            // get ready to try checking again
            intr_state = SREG;
            cli();
            UENUM = endpoint;
    }
    for (uint8_t i = keys_start; i < keys_end; i++) {
            UEDATX = report->raw[i];
    }
    UEINTX = 0x3A;
    SREG = intr_state;
    return 0;
}

static inline void uart_putc(uint8_t dat) {
        while (!( UCSR1A & (1 << UDRE1) ));
        UDR1 = dat;
}

static inline int8_t send_report_ble(report_keyboard_t *report, uint8_t endpoint, uint8_t keys_start, uint8_t keys_end) { 
    /**
     * Bluefruit LE UART implementation
     * device must be set to bluetooth HID for this to work
     */
    // Initialize service (again)
    uart_service_init();
    
    // print("Sending report via UART\n");
    // command mode for the GATT service on the bluetooth module
    // must end with dec 13 to process command
    // Transmit the first part of the command
    
    // sizeof - 1 so the null terminator isnt captured
    uart_xmit_str(gatt_cmd, sizeof(gatt_cmd) - 1);

    uint8_t char_buf[3];
    for (uint8_t i = keys_start; i < keys_end; i++) {
        // convert that hex value into a string then send the string
        sprintf(char_buf, "%02x", report->raw[i]);
        uart_xmit(char_buf[0]);
        uart_xmit(char_buf[1]);
        // separate all characters with a hyphen given index is not the last
        if (i < keys_end - 1)
            uart_xmit('-');
    }
    // Return character
    uart_xmit(0xd);
    
    return 0;
}
