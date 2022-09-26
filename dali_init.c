
#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "console.h"

// Good documentation for Dali commands - https://www.nxp.com/files-static/microcontrollers/doc/ref_manual/DRM004.pdf


void dali_init() {

    // Output is PB2 (WO2) - Initially low (which will be inverted by the transistor) 
    // whenever we're not waveform generating, the pin will return to this level.
    PORTB.OUTCLR = PIN2_bm;
    PORTB.DIRSET = PIN2_bm;

    // New Dali read pin is PA7 - used in AC mode, with a 0.55V reference voltage.
    PORTA.DIRCLR = PIN7_bm;
    // Analog comparator doesn't work unless you disable the port's GPIO.
    PORTA.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;

    // Set the Reference Voltage to 0.55V.  With voltage division, this equals a real threshold of 25.3/3.3 * 0.55 = 4.2V and consumes about 16V/25300 = 632uA
    VREF.CTRLA = VREF_DAC0REFSEL_1V1_gc;
    AC0.MUXCTRLA = AC_MUXPOS_PIN0_gc | AC_MUXNEG_VREF_gc;
    AC0.CTRLA = AC_RUNSTDBY_bm | AC_INTMODE_NEGEDGE_gc | AC_HYSMODE_50mV_gc | AC_ENABLE_bm; 


    // Set up event systemt to route AC events to channel 0.
    EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_AC0_OUT_gc;

    // Ensure the timer is stopped;
    TCA0.SINGLE.CTRLA = 0;     

    // Set up timer TCA0 for DALI transmission. 
    TCA0.SINGLE.EVCTRL  = 0;

    // We start in the reset state (which we should exit in short order).
    dali_wait_for_idle_state_enter();
    // Make the TCA.CMP1 interrupt the highest priority, so that it goes off ASAP after the timer goes off.
}

/**
 * @brief We may want to do different things with the AC interrupt.  This stores a pointer to the current thing.
 * May be NULL to do nothing. 
 * 
 */
void (*ac_isr_callback)() = NULL;


ISR(AC0_AC_vect) {
    if (ac_isr_callback) {
        ac_isr_callback();
    }
    AC0.STATUS = AC_CMP_bm; // Reset the Interrupt flag.
}



/**
 * @brief Reads the current DALI bus state.
 *  
 * NOTE: the AC must be enabled for this to work. 
 * 
 * @return uint8_t 1 for shorted, 0 for released...
 */
uint8_t dali_is_bus_shorted() {
    return  AC0.STATUS & AC_STATE_bm ? 0 : 1;
}



void dali_on_linechange(void (*callback)()) {
    AC0.INTCTRL = callback ? AC_CMP_bm : 0;
    ac_isr_callback = callback;
    AC0.STATUS = AC_CMP_bm; // Clear any pending interrupts.
}