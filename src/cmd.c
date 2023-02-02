



#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "config.h"
#include "cmd.h"

typedef enum {
    PULSE_HALF,
    PULSE_FULL,
    INVALID
} pulse_t;



static inline void write_one() {
    PORTB.OUTSET = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
    PORTB.OUTTGL = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
}

static inline void write_zero() {
    PORTB.OUTCLR = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
    PORTB.OUTTGL = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
}


static inline void write_byte(uint8_t byte) 
{
    for (uint8_t i = 0; i < 8; i++) {
        //write each bit 1 at a time.
        if (byte & 0x80) {
            write_one();
        } else {
            write_zero();
        }
        byte <<= 1;
    }
}

static read_result_t dali_write(uint8_t addr, uint8_t cmd) {
    // Check that line is high (and has been for some time?)
    // for each bit, drive low then high, or vice versa.
    if ((AC0.STATUS & AC_STATE_bm) == 0) {
        log_char('B');
        return READ_COLLISION;
    }

    // Write the start bit
    write_one();
    write_byte(addr);
    write_byte(cmd);
    
    PORTB.OUTCLR = PORT_INT2_bm;
    return READ_NAK;
}


static pulse_t read_pulse() {
    uint8_t lvl = AC0.STATUS & AC_STATE_bm;
    uint16_t t = 0;
    while ((AC0.STATUS & AC_STATE_bm) == lvl) {
        t = TCA0.SINGLE.CNT;
        if (t > USEC_TO_TICKS(DALI_BIT_USECS + DALI_MARGIN_USECS)) {
            return INVALID;
        }
    }
    TCA0.SINGLE.CNT = 0;
    if (t < USEC_TO_TICKS(DALI_HALF_BIT_USECS - DALI_MARGIN_USECS)) {
        return INVALID;
    }
    if (t < USEC_TO_TICKS(DALI_HALF_BIT_USECS + DALI_MARGIN_USECS)) {
        return PULSE_HALF;
    }
    if (t < USEC_TO_TICKS(DALI_BIT_USECS - DALI_MARGIN_USECS)) {
        return INVALID;
    }
    return PULSE_FULL;
}

    
static read_result_t dali_read(uint16_t timeout, uint8_t *out) {
    // disable interrupts. but why?
    // ensure line is high
    read_result_t result = READ_COLLISION;
    
    // Start TCA0, counting from 0, no interrupts, no fancy resetting.
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // start the clock

    // Wait for line to go low - record when this happened, then restart the clock
    while (AC0.STATUS & AC_STATE_bm) {
        // log_uint16("CLK", TCA0.SINGLE.CNT);
        if (TCA0.SINGLE.CNT >= timeout) {
            // Nothing received within timeout period.
            result = READ_NAK;
            goto cleanup;
        } 
    }
    TCA0.SINGLE.CNT = 0; // reset counting for the bit widths.
    // Read the first half of the start bit.
    if (read_pulse() != PULSE_HALF) {
        goto cleanup;
    }

    // We're now half way through the start bit.  Start reading pulses
    uint8_t val = 0;
    uint8_t last = 1;
    for (uint8_t i = 0; i < 8; i++) {
        switch (read_pulse()) {
            case PULSE_HALF:
                // We immediately expect another half pulse to take us back to the half bit, meaning we have a bit the same as the last one
                if (read_pulse() != PULSE_HALF) {
                    goto cleanup;
                }
                break;
            case PULSE_FULL:
                // Its a bit flip
                last = !last;
                break;
            default:
                goto cleanup;
        }
        val = val << 1 | last;
    }
    if (last == 0) {
        // We finished with a 0, which drives the line high, then low.  Read one last pulse in
        if (read_pulse() != PULSE_HALF) {
            goto cleanup;
        }
    }
    
    // For cleanliness' sake, we expect the bus to remain high for 2 bit periods after end
    while (TCA0.SINGLE.CNT < USEC_TO_TICKS(DALI_BIT_USECS*2)) {
        if ((AC0.STATUS & AC_STATE_bm) == 0) {
            goto cleanup;
        }
    }
    result = READ_VALUE;
    *out = val;
cleanup:
    // Stop TCA0
    // re-enable interrupts
    TCA0.SINGLE.CTRLA = 0;
    return result;
}


read_result_t send_dali_cmd(uint8_t addr, dali_gear_command_t cmd, uint8_t *out) {
    read_result_t res = dali_write(addr, cmd);
    if (res != READ_NAK) {
        return res;
    }
    // Give the line a chance to recover after transmitting before we start reading
    // There might be some propagation delay.
    _delay_us(10);
    res =  dali_read(USEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_USEC), out);

    // if a response was received, we can't transmit again for another 22 half bits (9.17ms)
    // Technically, we could set up a timer to go off after this time, but there's not really
    // much we can do during this time anyway, so we just delay here. 
    if (res == READ_VALUE) {
        _delay_us(DALI_RESPONSE_MAX_DELAY_USEC);
    }
    return res;
}
