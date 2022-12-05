#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "timers.h"
#include "console.h"

// Good documentation for Dali commands - https://www.nxp.com/files-static/microcontrollers/doc/ref_manual/DRM004.pdf


typedef enum {
    PULSE_TIMING,
    PULSE_HALF_BIT,
    PULSE_FULL_BIT,
    PULSE_INVALID_TOO_SHORT,
    PULSE_INVALID_MIDDLE,
    PULSE_INVALID_TOO_LONG,
} pulsewidth_t;

typedef void (*callback_t)(uint32_t data, uint8_t numBits);


static volatile uint32_t shiftreg = 0;
static volatile uint8_t numBits = 0;


// Indicates that the previous bit was a 1.  If not set, then the previous bit was a 0
#define PREVBIT1 0x01
// If set, we're half way through a bit.  If not, then we're between two bits.
#define AT_HALFBIT 0x02
// Sentinel value to indicate that we are waiting for the first half of the start bit.
#define IN_START 0xFF
// Combination of the above state masks.
static volatile uint8_t readerState;
callback_t callback;



static inline void shiftBitIn(uint8_t val) {
    shiftreg <<= 1 | val;
    numBits++;
}

static inline void cleanup() {
    cli();
    TCB0.CTRLA = 0;
    cancel_timeout();
    AC0.INTCTRL = 0;    // Turn off interrupts for AC0 - Note this does not turn off AC0.
    sei();
}


ISR(TCB0_INT_vect) {
    // TCB0's CNT has been reset automatically, but TCA0's hasn't 
    // We do this first so that it closely matches TCB0s as it can
    putchar('x');
    reset_timeout();
    // Flip the edge type we're looking for.
    TCB0.EVCTRL ^= TCB_EDGE_bm; // 1 is negative edge, 0 is positive edge
    uint16_t pulse_length = TCB0.CCMP;  // This will clear the interrupt flag.

    // Now process our pulse.
    bool fullBitPulseDuration;
    if (pulse_length < HALFTICK_MAX_TICKS) {
        // Pulse is too short
        goto error;
    } else if (pulse_length <= HALFTICK_MAX_TICKS) {
        fullBitPulseDuration = false;
    } else if (pulse_length <= FULLTICK_MIN_TICKS) {
        // pulse width is too long for a half bit, and too short for a full bit
        goto error;
    } else if (pulse_length <= FULLTICK_MAX_TICKS) {
        fullBitPulseDuration = true;
    } else {
        // pulse is too long - Should never happen as the timer will go off first.
        goto error;
    }

    if (readerState == IN_START) {
        // We only accept a half bit pulse while in the start bit
        if(fullBitPulseDuration) {
            goto error;
        }
        // We're now half way through the start bit.  What happens next depends on the next pulse.
        readerState = AT_HALFBIT | PREVBIT1;
    } else if (readerState & AT_HALFBIT) {
        if (fullBitPulseDuration) {
            // We have a new bit, the opposite of the last one, and we remain at the half bit.
            readerState ^= PREVBIT1; // toggle PREVBIT
            shiftBitIn(readerState & PREVBIT1); // and add it to the shift register.
        } else {
            // The next bit is the same bit as the last, but we can't push yet as it might be the last bit in the sequence.
            readerState &= ~AT_HALFBIT; // Clear the halfbit flag to indicate we're now between bits.
        }
    } else {
        // We are between bits, and the next bit must be a half bit (and the same bit as before)
        if (fullBitPulseDuration) {
            goto error;
        }
        shiftBitIn(readerState & PREVBIT1);            
        readerState |= AT_HALFBIT;
    }

    // timeout depends on whether we are potentially at the end sequence or not.
    switch (readerState & 0x03) {
        case (AT_HALFBIT | PREVBIT1): 
            update_timeout(PULSE_FULL_BIT*5/2);
            break;
        case 0:  // AT_ENDBIT and PREVBIT0
            update_timeout(PULSE_FULL_BIT*2);
            break;
        default:
            // we're in a position where anthing longer than a full bit period would be invalid.
            update_timeout(PULSE_INVALID_TOO_LONG);  
    }

    return;  // Avoid the error handler. 
error:
    // We received an invalid pulse width. 
    log_info("Error");
    cleanup();
    dali_wait_for_idle_state_enter();
}

// TODO TCA0_OVF is re-used - come up with a demux.
void timerOverflow() {
    // This indicates that a timeout has occurred without a bit flip. This can either be stop bits or a timing error/
    // In either case, we will be leaving this state.  Shut down the timers.
    cleanup();

    if (TCA0.SINGLE.CNT >= PULSE_FULL_BIT*2 && (numBits == 8 || numBits == 16 || numBits == 24)) {
        // We have come to a valid end of sequence.  Process what we have read.
        callback(shiftreg, numBits);
        numBits = 0;
        shiftreg = 0;
    } else {
        // Some timing error or collision occurred.  
        log_info("Too long");

        dali_wait_for_idle_state_enter();
    }
}




void dali_state_receiving_prepare(callback_t callback) {
    log_info("rcvprep");

    shiftreg = 0;
    readerState = IN_START;
    numBits = 0;

    // We'll be using TCB0 to read pulse widths. 
    TCB0.CNT = 3;
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm; // Waiting for a positive edge to start with.

    // We'll also use TCA0 to capture timeouts. We expect each pulse to be either PULSE_HALF_BIT or PULSE_FULL_BIT long.
    // If we get a value longer than PULSE_INVALID_TOO_LONG then we enter into an error state.

    // Configure, but do not turn on, the timer. 
    on_timeout(PULSE_INVALID_TOO_LONG, timerOverflow);
}


/**
 * @brief Start receiving a dali waveform.
 * This method is called after receiving the falling edge of a start bit, so we expect the bus to be
 * low (shorted) at this point
 * 
 * We also expect dali_state_receiving_prepare to have been called before this one.
 */
void dali_state_receiving_enter() {
    log_info("receiving");
    AC0.STATUS = AC_CMP_bm; // Remove any existing interrupt flags.
    AC0.INTCTRL = AC_CMP_bm; // Turn on interrupts for AC0.
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // Start TCB0 for pulse width timing
    start_timeout();
}
