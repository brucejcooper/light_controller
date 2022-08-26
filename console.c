
#include "console.h"
#include "stdio.h"
#include "avr/io.h"
#include "avr/interrupt.h"



#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

int printCHAR(char character, FILE *stream);
static FILE mystdout = FDEV_SETUP_STREAM(printCHAR, NULL, _FDEV_SETUP_WRITE);


int printCHAR(char character, FILE *stream) {
    character = character % 0x7F;
    USART0_sendChar(character);
    return 0;
}


void USART0_sendChar(const char c) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {
        ;
    }
    USART0.TXDATAL = c;
}

int printCHAR(char character, FILE *stream);


void log_info(char *str, ...) {
    va_list ap;

    va_start(ap, str);

    vprintf(str, ap);
    USART0_sendChar('\r');
    USART0_sendChar('\n');
    va_end(ap);
}



void console_init() {
    // Use alternate pins, so we don't clash with TCA0
    PORTMUX.CTRLB |= PORTMUX_USART0_bm;

    // Configure TX pin as an output, defaulting to high.
    PORTA.DIRSET = PIN1_bm;

    // Baud rate.
    USART0.BAUD = USART0_BAUD_RATE(9600);
    // N81
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc |  USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc | USART_SBMODE_1BIT_gc;

    // Enable TX and RX
    USART0.CTRLB |= USART_TXEN_bm; // USART_TXEN_bm | USART_RXEN_bm;

    // Switch stdout for our character printing version.
    stdout = &mystdout;
}