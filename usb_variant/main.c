#define __DELAY_BACKWARD_COMPATIBLE__

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "../console.h"

#define BIT_USEC 833

#define HALF_BIT_USEC (BIT_USEC/2) 
typedef void (*callback_t)(int16_t val);

static callback_t pinchangeCallback = NULL;



static inline void transmitBit(uint8_t bit) {
    if (bit) {
        PORTB.OUTSET = PORT_INT2_bm;
    } else {
        PORTB.OUTCLR = PORT_INT2_bm;
    }
    _delay_us(HALF_BIT_USEC-2);
    PORTB.OUTTGL = PORT_INT2_bm;
    _delay_us(HALF_BIT_USEC-4);
}

static inline uint8_t isShorted() {
    return PORTA.IN;
}

// static uint16_t receive() {
//     bool shorted = wait_for_short();
// }

static bool wait_for_short(uint16_t timeout_usec) {
    // Turn on a timer. 
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;

    uint16_t timeout = timeout_usec*10/3; // for 3.33mhz clock.
    bool timedOut;

    while (!isShorted() && (timedOut = (TCA0.SINGLE.CNT <= timeout))) {
        ; // Do nothing.
    }

    TCA0.SINGLE.CTRLA = 0;
    TCA0.SINGLE.CNT = 0;
    return !timedOut;
}


ISR(PORTA_PORT_vect) {
    uint16_t t = TCA0.SINGLE.CNT;
    TCA0.SINGLE.CNT = 0;
    if (PORTA.INTFLAGS & PORT_INT7_bm) {
        // Pin change happened.
        PORTA.INTFLAGS  = PORT_INT7_bm;

        if (pinchangeCallback) {
            pinchangeCallback((int16_t) t);
        }
    }
}

ISR(TCA0_CMP0_vect) {
    TCA0.SINGLE.CNT = 0;
    // Timeout occurred.
    pinchangeCallback(-1);
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}


void on(uint8_t isc, uint16_t timeout, callback_t callback) {
    pinchangeCallback = callback;
    PORTA.PIN7CTRL = isc;
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.CMP0 = timeout*10/3;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_ENABLE_bm; // Turn clock on (if it isn't already)
    // TCA0.SINGLE.INTCTRL = TCA
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm; // Clear any old interrupts. 
}


static uint16_t transmit(uint32_t val, uint8_t numBits) {
    transmitBit(1); // Start Bits.
    val <<= (32-numBits);
    for (uint8_t i = 0; i < numBits; i++) {
        transmitBit(val & 0x80000000 ? 1 : 0);
        val <<= 1;
    }
    PORTB.OUTCLR = PORT_INT2_bm;
    _delay_us(BIT_USEC*2); // Stop Bits.

    // Ignore everything for 7 Half bits.  If something happens during this time, its illegal. 
    _delay_us(HALF_BIT_USEC*7); // Stop Bits.
    // Wait for up to another 15 half bit time periods (total of 22) for a response to start.

    return 0; //receive();
}




int main(void) {
    console_init();
    sei();

    log_info("Welcome");

    // Set PB2 as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Set PA7  as in input, with pullup.
    // PORTA.PIN7CTRL = PORT_PULLUPEN_bm;
    PORTA.DIRCLR = PORT_INT7_bm;

    uint16_t data = 0xFEE1;


    while (1) {
        log_info("transmitting");

        transmit(data, 16);
        _delay_ms(100);
    }
    return 0;
}
