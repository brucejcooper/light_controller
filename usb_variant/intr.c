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

static isr_handler_t tca0_cmp_handler = NULL;
static isr_handler_t tca0_ovf_handler = NULL;
static isr_pulse_handler_t tcb0_handler = NULL;

void set_input_pulse_handler(isr_pulse_handler_t tcb0) {
    tcb0_handler = tcb0;
}

void set_output_pulse_handler(isr_handler_t h) {
    tca0_cmp_handler = h;
}


void set_isrs(isr_handler_t tca0_cmp, isr_handler_t tca0_ovf, isr_pulse_handler_t tcb0) {
    // clear any existing ISRS
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm | TCA_SINGLE_CMP2_bm | TCA_SINGLE_OVF_bm;
    TCB0.INTFLAGS = TCB_CAPT_bm;

    uint8_t mask = 0;
    tca0_cmp_handler = tca0_cmp;
    tca0_ovf_handler = tca0_ovf;
    tcb0_handler = tcb0;

    if (tca0_cmp_handler) {
        mask |= TCA_SINGLE_CMP0_bm;
    }
    if (tca0_ovf_handler) {
        mask |= TCA_SINGLE_OVF_bm;
    }
    TCA0.SINGLE.INTCTRL = mask;

    TCB0.INTCTRL = tcb0_handler ? TCB_CAPT_bm : 0;
}


ISR(TCA0_CMP0_vect) {
    if (tca0_cmp_handler) {
        tca0_cmp_handler();
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm; // Clear flag

}


ISR(TCA0_OVF_vect) {
    if (tca0_ovf_handler) {
        tca0_ovf_handler();
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear flag
}


ISR(TCB0_INT_vect) {
    uint16_t cnt = TCB0.CCMP; // This will clear the interrupt flag.
    TCB0.EVCTRL ^= TCB_EDGE_bm;  // We always toggle the edge we're looking for. 
    TCA0.SINGLE.CNT = 0; // Reset Timeout clock

    if (tcb0_handler) {
        tcb0_handler(cnt);
    }
}