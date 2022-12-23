
#include "console.h"
#include "avr/io.h"
#include "avr/interrupt.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)
#define OUTBUF_SZ 60



typedef struct {
    char buf[OUTBUF_SZ];
    uint8_t write;
    uint8_t read;
} circular_log_char_t;

static circular_log_char_t out = {
    .write = 0,
    .read = 0,
};


static void buf_write(circular_log_char_t *buf, char data) {
    buf->buf[buf->write] = data;
    buf->write = (buf->write + 1) % OUTBUF_SZ;
    if (buf->write == buf->read) {
        // Overflow. We deal with this by discarding the oldest readable value.
        buf->read = (buf->read + 1) % OUTBUF_SZ;
    }
}

static bool buf_read(circular_log_char_t *buf, char *data) {
    if (buf->read == buf->write) {
        // There's nothing in the log_char.
        return false;
    }
    // There _is_ something in the log_char. copy it into the pointer. 
    *data = buf->buf[buf->read];
    buf->read = (buf->read + 1) % OUTBUF_SZ;
    return true;
}


static void push_from_out_buf_to_uart() {
    char ch;
    if (buf_read(&out, &ch)) {
        USART0.TXDATAL = ch;
        USART0.CTRLA = USART_DREIE_bm; // Enable write interrupt (if it wasn't already). 
    } else {
        USART0.CTRLA = 0; // Disable write interrupt, as we don't have anything to transmit. 
    }
}


void log_char(char character) {
    buf_write(&out, character);
    if (USART0.STATUS & USART_DREIF_bm) {
        push_from_out_buf_to_uart();
    }
}



static void myPuts(char *str) {
    while (*str) {
        log_char(*str++);
    }
}

static inline void outputHexNibble(uint8_t nibble) {
    if (nibble < 10) {
        log_char('0' + nibble);
    } else if (nibble < 16) {
        log_char('a' + nibble - 10);
    } else {
        log_char('!');
    }
}

static void log_hex(uint8_t val) {
    outputHexNibble(val >> 4);
    outputHexNibble(val & 0x0F);
}

void log_uint24(char *str, uint32_t val) {
    myPuts(str);
    myPuts(" 0x");
    log_hex((uint8_t) (val >> 16));
    log_hex((uint8_t) (val >> 8));
    log_hex((uint8_t) (val));
    myPuts("\r\n");
    
}

void log_uint16(char *str, uint16_t val) {
    myPuts(str);
    myPuts(" 0x");
    log_hex(val >> 8);
    log_hex(val);
    myPuts("\r\n");
    
}

void log_uint8(char *str, uint8_t val) {
    myPuts(str);
    myPuts(" 0x");
    log_hex(val);
    myPuts("\r\n");
    
}

void log_val(uint32_t val, uint8_t len) {
    for (int8_t i = len-8; i >= 0; i -= 8) {
        log_hex(val >> i);
    }
}

void log_cmd(command_event_t *cmd) {
    log_char('\r');
    log_val(cmd->val, cmd->len);
    switch (cmd->outcome) {
        case RESPONSE_RESPOND:
            myPuts(" >");
            log_char(' ');
            log_hex(cmd->response.mine.val);
            break;
        case RESPONSE_NACK:
            myPuts(" >");
            myPuts(" (NAK)");
            break;
        case RESPONSE_IGNORE:
            myPuts(" <");
            switch (cmd->response.other.type) {
                case RECEIVE_EVT_RECEIVED:
                    log_char(' ');
                    log_val(cmd->response.other.rcv.data, cmd->response.other.rcv.numBits);
                    break;
                case RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD:
                    myPuts(" (NAK)");
                    break;
                case RECEIVE_EVT_INVALID_SEQUENCE:
                    myPuts(" INV width ");
                    log_hex(cmd->response.other.rcv.data >> 8);
                    log_hex(cmd->response.other.rcv.data);
                    myPuts(" pos ");
                    log_hex(cmd->response.other.rcv.numBits);
                    break;
            }
            break;
        case RESPONSE_REPEAT:
            myPuts(" REP");
            break;
        case RESPONSE_INVALID_INPUT:
            myPuts(" INVALID");
            break;
    }
    myPuts("\r\n");
    
}


void log_info(char *str) {
    myPuts(str);
    myPuts("\r\n");
    
}

ISR(USART0_DRE_vect) {
    // Called when Transmit log_char is ready to receive new data. 
    push_from_out_buf_to_uart();
    USART0.STATUS = USART_DREIF_bm;
}



void console_init() {

    out.buf[0] = 0;

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
    USART0.CTRLA = 0; // Enable read interrupt only to start with - TX interrupt will be enabled when there is data to transmit. 

    // Enable TX and RX
    USART0.CTRLB |= USART_TXEN_bm;

    myPuts("\r\nSystem Started\r\n");
}