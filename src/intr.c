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


void startSingleShotTimer(uint16_t to, isr_handler_t callback) {
    timeout_handler = callback;
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear any existing flag
    TCA0.SINGLE.INTCTRL |= TCA_SINGLE_OVF_bm;
    if (TCA0.SINGLE.CTRLA == 0) {
        TCA0.SINGLE.CNT = 0;  // We try to always reset timer counters, but make sure. 
    }
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
    timeout_handler = NULL;
    // callback = NULL;
    // Stop the timer
    // clearTimeout();
    // Call the handler
    if (callback) {
        callback();
    }
    // If the Handler re-instated a timer or they started transmitting (changing the mode to non NORMAL) 
    // we keep the timer going from where it was.
    // If nothing was (re)started, we stop the timer.
    if (!timeout_handler && (TCA0.SINGLE.CTRLB && TCA_SINGLE_WGMODE_gm) == TCA_SINGLE_WGMODE_NORMAL_gc) {
        clearTimeout();
    }
}
