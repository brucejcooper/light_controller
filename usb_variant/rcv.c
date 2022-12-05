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
#include <stdlib.h>
#include <string.h>
#include "../console.h"
#include "intr.h"
#include "rcv.h"
#include "timing.h"
#include "idle.h"


static uint32_t shiftReg;
static uint8_t numBits = 0;
static uint8_t lastBit;
static bool valid;

static void pulse_after_half_bit(uint16_t pulseWidth);
static void pulse_after_bit_boundary(uint16_t pulseWidth);


static inline void pushBit() {
    // putchar(lastBit ? '1': 0);
    shiftReg = shiftReg << 1 | lastBit;
    numBits++;
}


static void invlalid_sequence(uint16_t pulseWidth) {
    // Don't do anything...
}

static void start_bit_received(uint16_t pulseWidth) {
    if (!isHalfBit(pulseWidth)) {
        log_info("start bit wasn't a half bit");
        set_input_pulse_handler(invlalid_sequence);
    };
    // putchar('S');
    lastBit = 1;
    set_input_pulse_handler(pulse_after_half_bit);
}


static void pulse_after_half_bit(uint16_t pulseWidth) {
    if (isHalfBit(pulseWidth)) {
        // putchar('h');
        set_input_pulse_handler(pulse_after_bit_boundary);
    } else if (isFullBit(pulseWidth)) {
        // putchar('f');
        lastBit = lastBit ? 0 : 1;
        pushBit();
    } else {
        log_info("invalid pulse width %d", pulseWidth);
        set_input_pulse_handler(invlalid_sequence);

    }
    
}

static void pulse_after_bit_boundary(uint16_t pulseWidth) {
    if (!isHalfBit(pulseWidth)) {
        log_info("Pulse from bit boundary that wasn't a half bit");
        set_input_pulse_handler(invlalid_sequence);
    } else {
        // putchar('H');
        pushBit();
        set_input_pulse_handler(pulse_after_half_bit);
    }
}


static void timeout() {
    TCA0.SINGLE.CTRLA = 0;
    TCB0.CTRLA = 0; // Stop TCB0
    // printf("T\r\n");


    if (!(AC0.STATUS & AC_STATE_bm)) {
        // Timeout occurred while shorted.  This shouldn't happen.
        log_info("Timeout while shorted");
    }

    if (valid) {
        log_info("Read 0x%08lx(%u)", shiftReg, numBits);
    } else {
        log_info("Ignoring invalid input");
    }    
    idle_enter();
}
    

void read_enter(uint16_t _ignored) {
    // TCB0 should already be running, in Frequency Capture mode. 
    if (TCB0.CTRLA != (TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm) || TCB0.CTRLB != TCB_CNTMODE_FRQ_gc || TCB0.EVCTRL != (TCB_CAPTEI_bm)) {
        log_info("Entering read when not in the right mode. In %02x,%02x,%02x", TCB0.CTRLA, TCB0.CTRLB, TCB0.EVCTRL);
    }
    
    shiftReg = 0;
    valid = true;
    numBits = 0;
        
    // Start a timer that will go off after the maximum wait time (2 Bit periods) to indicate we're done.
    TCA0.SINGLE.CTRLA = 0;
    TCA0.SINGLE.CNT = 0;  // We try to always reset timer counters, but make sure. 
    TCA0.SINGLE.PER = USEC_TO_TICKS(2*DALI_BIT_USECS); // Send ISR when clock reaches PER.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // normal counter mode, no output.
    TCA0.SINGLE.CTRLA =  TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; // Make the timeout clock run

    set_isrs(NULL, timeout, start_bit_received);

}