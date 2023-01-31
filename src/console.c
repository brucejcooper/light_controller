
#include "console.h"
#include "avr/io.h"
#include "avr/interrupt.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

void log_char(char character) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {
        ;
    }
    USART0.TXDATAL = character;
}

static void myPuts(char *str) {
    while (*str) {
        log_char(*str++);
    }
}

static inline void outputHexNibble(uint8_t nibble) {
    log_char(nibble < 0x0a ? '0' + nibble : 'a' + nibble - 10);
}

void log_hex(uint8_t val) {
    outputHexNibble(val >> 4);
    outputHexNibble(val & 0x0F);
}

void log_uint24(char *str, uint32_t val) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts(" 0x");
    log_hex(val >> 16);
    log_hex(val >> 8);
    log_hex(val);
    myPuts("\r\n");
}


void log_uint16(char *str, uint16_t val) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts(" 0x");
    log_hex(val >> 8);
    log_hex(val);
    myPuts("\r\n");
}

void log_uint8(char *str, uint8_t val) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts(" 0x");
    log_hex(val);
    myPuts("\r\n");
}


void log_info(char *str) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts("\r\n");
}

void console_flush() {
    while (!(USART0.STATUS & USART_TXCIF_bm)) {
        ;
    }
}

void console_init() {
    // Use alternate pins, so we don't clash with TCA0
    PORTMUX.CTRLB |= PORTMUX_USART0_bm;

    // Configure TX (PA1) pin as an output, defaulting to high.
    PORTA.OUTSET = PIN1_bm;
    PORTA.DIRSET = PIN1_bm;

    // Configure RX (PA2) as an input
    PORTA.DIRCLR = PIN2_bm;

    // Baud rate.
    USART0.BAUD = USART0_BAUD_RATE(115200);
    // N81
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc |  USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc | USART_SBMODE_1BIT_gc;
    USART0.CTRLA = USART_RXCIE_bm; // Enable read interrupt only to start with - TX interrupt will be enabled when there is data to transmit. 

    // Enable TX
    USART0.CTRLB |= USART_TXEN_bm;

    myPuts("\r\nSystem Started\r\n");
}