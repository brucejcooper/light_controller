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

#include "intr.h"

static isr_handler_t timeout_handler = NULL;
static isr_pulse_handler_t tcb0_handler = NULL;

void set_input_pulse_handler(isr_pulse_handler_t tcb0) {
    tcb0_handler = tcb0;
}

// TODO this and the above are basically the same thing.  Refactor.
void set_incoming_pulse_handler(isr_pulse_handler_t tcb0) {
    // clear any existing ISRS
    TCB0.INTFLAGS = TCB_CAPT_bm;
    tcb0_handler = tcb0;
    TCB0.INTCTRL = tcb0_handler ? TCB_CAPT_bm : 0;
}


void startSingleShotTimer(uint16_t to, isr_handler_t callback) {
    timeout_handler = callback;
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear any existing flag
    TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
    TCA0.SINGLE.CNT = 0;  // We try to always reset timer counters, but make sure. 
    TCA0.SINGLE.PER = to; // Send ISR when clock reaches PER.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // normal counter mode, no output.
    TCA0.SINGLE.CTRLA =  TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; // Make the timeout clock run
}

void clearTimeout() {
    TCA0.SINGLE.CTRLA = 0;
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear any existing flag
    TCA0.SINGLE.INTCTRL &= ~TCA_SINGLE_OVF_bm;
    timeout_handler = NULL;
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.PER = 0xFFFF;
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; 
}



ISR(TCA0_OVF_vect) {
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear flag

    isr_handler_t callback = timeout_handler;
    // Stop the timer
    clearTimeout();
    // Call the handler
    if (callback) {
        callback();
    }
}


ISR(TCB0_INT_vect) {
    uint16_t cnt = TCB0.CCMP; // This will clear the interrupt flag.
    TCB0.EVCTRL ^= TCB_EDGE_bm;  // We always toggle the edge we're looking for. 
    TCA0.SINGLE.CNT = 0; // Reset Timeout clock

    if (tcb0_handler) {
        tcb0_handler(cnt);
    }
}