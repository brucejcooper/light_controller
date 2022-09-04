#define __DELAY_BACKWARD_COMPATIBLE__

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"

// Good documentation for Dali commands - https://www.nxp.com/files-static/microcontrollers/doc/ref_manual/DRM004.pdf


typedef enum {
    PULSE_TIMING,
    PULSE_HALF_BIT,
    PULSE_FULL_BIT,
    PULSE_INVALID_TOO_SHORT,
    PULSE_INVALID_MIDDLE,
    PULSE_INVALID_TOO_LONG,
} pulsewidth_t;

static volatile pulsewidth_t current_pulse_width = PULSE_TIMING;

// Use TCB in Input Capture Frequency Measurement mode.  This will generate a CAPT interrupt on edge transition, and copy
// the clock value into CCMP.  This will result in accurate pulse width measurement.
// How to deal with a pulse that goes on for a long high pulse (like a stop condition) though?
// Have the main loop check to see if CNT goes above the too long threhsold.  When it does, disable the counter and proceed as if TOO_LONG


ISR(TCB0_INT_vect)
{
    uint16_t pulse_length = TCB0.CCMP;  // This will clear the interrupt flag.
    
    if (pulse_length < HALFTICK_MAX_TICKS) {
        current_pulse_width = PULSE_INVALID_TOO_SHORT;
    } else if (pulse_length <= HALFTICK_MAX_TICKS) {
        current_pulse_width = PULSE_HALF_BIT;
    } else if (pulse_length <= FULLTICK_MIN_TICKS) {
        current_pulse_width = PULSE_INVALID_TOO_SHORT;
    } else if (pulse_length <= FULLTICK_MAX_TICKS) {
        current_pulse_width = PULSE_FULL_BIT;
    } else {
        current_pulse_width = PULSE_INVALID_TOO_LONG;
    }

    // Flip the edge type we're looking for.
    TCB0.EVCTRL ^= TCB_EDGE_bm; // 1 is negative edge, 0 is positive edge
}


uint8_t dali_read_bus() {
    return DALI_PORT.IN & DALI_RX_bm ? 1 : 0;
}


pulsewidth_t time_pulse(uint8_t *current_val) {    
    // is there a race condition here on compare to CNT?  If there is, it'll just loop again, and you'll be fine.
    for (current_pulse_width = PULSE_TIMING; current_pulse_width == PULSE_TIMING && TCB0.CNT < FULLTICK_MAX_TICKS; ) { 
        // We're just waiting, so do nothing.
    }
    pulsewidth_t result = current_pulse_width == PULSE_TIMING ? PULSE_INVALID_TOO_LONG : current_pulse_width;
    current_pulse_width = PULSE_TIMING;
    return result;
}


dali_result_t dali_receive(uint8_t *address, uint8_t *command) {
    dali_result_t res = DALI_CORRUPT_READ;
    uint8_t last_bit = 1;
    uint8_t current_val = 0;
    uint16_t shiftreg = 0;
    pulsewidth_t pulse_width;

    // Check to see if the line has been pulled down (indicating the start of a transmission)
    if (DALI_PORT.IN & DALI_RX_bm) {
        return DALI_NO_START_BIT;
    }

    // We know we're low at this point, which is hopefully the start of a transmission. Set up the timer.
    TCB0.CNT = 3;
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm; // Waiting for a positive edge to start with.
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;


    // Wait for first half bit of the start bit, which must be low for one half bit period.
    pulse_width = time_pulse(&current_val);
    if (pulse_width != PULSE_HALF_BIT) {
        goto cleanup;
    }

    // Now we're half way through the start bit.
    while (res != DALI_OK) {
        // At the top of the loop we are always a half bit period into a bit.
        pulse_width = time_pulse(&current_val);

        if (pulse_width == PULSE_HALF_BIT) {
             // There must be a followup half tick sized pulse (unless line has been high for a while, and the last bit was a zero, indicating an end of transmission)
             pulse_width = time_pulse(&current_val);

             if (pulse_width == PULSE_HALF_BIT) {
                shiftreg = (shiftreg << 1) | last_bit;    
             } else if (pulse_width == PULSE_INVALID_TOO_LONG && current_val > 0 && last_bit == 0) {
                // that was the end of the packet.
                res = DALI_OK;
             }
        } else if (pulse_width == PULSE_FULL_BIT) {
            // Its a bit toggle, and we remain in the half bit position.
            last_bit = last_bit ? 0 : 1;
            shiftreg = (shiftreg << 1) | last_bit;
        } else if (pulse_width == PULSE_INVALID_TOO_LONG && current_val > 0 && last_bit == 1) {
            // The end of the packet.
            res = DALI_OK;
        } else {
            // Its an invalid pulse
            goto cleanup;
        }
    } 

    // If we get here, we've already waited for the line to be high for 1 bit period, but we now need to wait for one more
cleanup:
    TCB0.CTRLA = 0; // Disable TCB0
    // wait for line to be high for at least x ms
    // step 1 - wait for it to be high.
    while ((DALI_PORT.IN & DALI_RX_bm) == 0) {
        // Do nothing.
    }
    _delay_ms(1); // Poor mans way of waiting for line...
    return res;
}
