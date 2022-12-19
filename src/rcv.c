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
#include "console.h"
#include "intr.h"
#include "rcv.h"
#include "timing.h"
#include "state_machine.h"


static receive_cb_t current_callback;
static receive_event_t evt;
static uint8_t lastBit;

static void pulse_after_half_bit(uint16_t pulseWidth);
static void pulse_after_bit_boundary(uint16_t pulseWidth);
static void startbit_started(uint16_t _ignored);

static inline void pushBit() {
    // putchar(lastBit ? '1': 0);
    evt.rcv.data = evt.rcv.data << 1 | lastBit;
    evt.rcv.numBits++;
}


static void pulse_after_invalid(uint16_t pulseWidth) {
    // All subsequent pulses until timeout are ignored. 
}

static void invalid_sequence_received() {
    evt.type = RECEIVE_EVT_INVALID_SEQUENCE;
    set_input_pulse_handler(pulse_after_invalid);
}

static void pulse_after_start(uint16_t pulseWidth) {
    if (!isHalfBit(pulseWidth)) {
        log_info("start bit wasn't a half bit");
        invalid_sequence_received();
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
        invalid_sequence_received();
    }
}

static void pulse_after_bit_boundary(uint16_t pulseWidth) {
    if (!isHalfBit(pulseWidth)) {
        log_info("Pulse from bit boundary that wasn't a half bit");
        invalid_sequence_received();
    } else {
        // putchar('H');
        pushBit();
        set_input_pulse_handler(pulse_after_half_bit);
    }
}


static void timeout_occurred() {
    clearTimeout(); 
    TCB0.CTRLA = 0; // Stop TCB0
    set_isrs(NULL, NULL); // Turn off all ISRs.
    // printf("T\r\n");

    if (!(AC0.STATUS & AC_STATE_bm)) {
        // Timeout occurred while shorted.  This shouldn't happen.
        log_info("Timeout while shorted");
    }
    current_callback(&evt);
    current_callback = NULL;
    // idle_enter();
}
    

void waitForRead(uint16_t timeout, receive_cb_t cb) {
    current_callback = cb;
    evt.rcv.data = 0;
    evt.rcv.numBits = 0;
    evt.type = RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD;

    // Disable TCA0 initially.
    TCA0.SINGLE.CTRLA = 0;
    if (timeout > 0) {
        startSingleShotTimer(timeout, timeout_occurred);
    }
    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Waiting for a negative edge to start with.
    TCB0.INTFLAGS = TCB_CAPT_bm; // Clear any existing interrupt.
    TCB0.INTCTRL = TCB_CAPT_bm; // Enable Interrupts on CAPTURE
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // Start TCB0 for pulse width timing - First pulse should reset clock.
    set_isrs(NULL, startbit_started);
}

void startbit_started(uint16_t _ignored) {
    setCanTransmitImmediately(false);
    // TCB0 should already be running, in Frequency Capture mode. 
    if (TCB0.CTRLA != (TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm) || TCB0.CTRLB != TCB_CNTMODE_FRQ_gc || TCB0.EVCTRL != (TCB_CAPTEI_bm)) {
        log_info("Entering read when not in the right mode. In %02x,%02x,%02x", TCB0.CTRLA, TCB0.CTRLB, TCB0.EVCTRL);
    }
    set_isrs(NULL, pulse_after_start);
    // Start a timer that will go off after the maximum wait time (2 Bit periods) to indicate we're done.
    startSingleShotTimer(USEC_TO_TICKS(2*DALI_BIT_USECS), timeout_occurred); // We are done when no pulse is received within 2 BIT periods.
}