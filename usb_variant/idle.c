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



extern void idle_enter() {
    TCA0.SINGLE.CTRLA = 0;    // TCA0 is disabled.
    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Reset the Mode by going to INT mode before going into the FRQ mode.
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Waiting for a negative edge to start with.
    TCB0.INTFLAGS = TCB_CAPT_bm; // Clear any existing interrupt.
    TCB0.INTCTRL = TCB_CAPT_bm; // Enable Interrupts on CAPTURE
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // Start TCB0 for pulse width timing - First pulse should reset clock.
    set_isrs(NULL, NULL, read_enter);
}