
#include "console.h"
#include "avr/io.h"
#include "avr/interrupt.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)
#define INBUF_SZ 64


static command_hanlder_t cmdHandler = NULL;
static char in[INBUF_SZ];
static char *inPtr = in;

void log_char(char character) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {
        ;
    }
    USART0.TXDATAL = character;
    USART0.CTRLA = USART_RXCIE_bm; // Enable write interrupt (if it wasn't already). 
}

static void myPuts(char *str) {
    while (*str) {
        log_char(*str++);
    }
}

void printPrompt() {
    *inPtr = '\0';
    log_char('\r');
    log_char('>');
    log_char(' ');
    myPuts(in);
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
    printPrompt();
}


void log_uint16(char *str, uint16_t val) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts(" 0x");
    log_hex(val >> 8);
    log_hex(val);
    myPuts("\r\n");
    printPrompt();
}

void log_uint8(char *str, uint8_t val) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts(" 0x");
    log_hex(val);
    myPuts("\r\n");
    printPrompt();
}


void log_info(char *str) {
    myPuts("\r\033[2K");
    myPuts(str);
    myPuts("\r\n");
    printPrompt();
}

static void clearInputBuffer() {
    inPtr = in;
    *inPtr=  '\0';
}


void sendCommandResponse(command_response_t response) {
    switch (response) {
        case CMD_DEFERRED_RESPONSE:
            // Do nothing right now, because we expect a later system to call this again with the real response.
            return;
        case CMD_OK: 
            myPuts("\rOK\r\n");        
            break;
        case CMD_NO_OP: 
            myPuts("\rNOOP\r\n");        
            break;
        case CMD_BAD_INPUT: 
            myPuts("\rBad Input\r\n");        
            break;
        case CMD_FAIL: 
            myPuts("\rFailed\r\n");        
            break;
        case CMD_FULL: 
            myPuts("\rFull\r\n");        
            break;
        case CMD_BUSY:
            myPuts("\rBusy\r\n");        
            break;
    }
    printPrompt();

}

ISR(USART0_RXC_vect) {
    // Called when Something happens to the receive buffer. The only interrupt we support is RXC, so clear it. 
    USART0.STATUS = USART_RXCIF_bm;

    char ch = USART0.RXDATAL;

    if ((inPtr-in) == INBUF_SZ) {
        inPtr = in; // Reset it back to an empty command. 
        puts("OVERFLOW\r>");
    }

    if (ch == '\r') {
        log_char('\r'); 
        log_char('\n'); // add a newline to put us on the next line.
        *inPtr = '\0';
        
        command_response_t response = cmdHandler ? cmdHandler(in) : CMD_NO_OP;
        clearInputBuffer();
        if (response != CMD_DEFERRED_RESPONSE) {
            sendCommandResponse(response);
        }
        
    } else if (ch == 8 || ch == 127) {
        // Backspace
        if (inPtr > in) {
            inPtr--;
            log_char(8);
            log_char(' ');
            log_char(8);
        }
    } else {
        log_char(ch); // Echo it back out to the client.
        *inPtr++ = ch;
    }
}


void console_init(command_hanlder_t handler) {
    in[0] = 0;
    inPtr = in;
    cmdHandler = handler;


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

    // Enable TX and RX
    USART0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;

    myPuts("\r\nSystem Started\r\n");
    printPrompt();
}