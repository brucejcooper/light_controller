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
#include "console.h"

// Good documentation for Dali commands - https://www.nxp.com/files-static/microcontrollers/doc/ref_manual/DRM004.pdf


// At maximum, we need two pulses for each bit, pluse one extra for the final clean up, plus one stop half bit, plus stop bits.
static volatile uint16_t pulsePeriods[35];
static volatile uint16_t *pulsePtr;
volatile bool dali_transmitting = false;

/** We want to know if we're currently outputting a short or a release, so that we can do collision detection when released
 * To do this, we track the current output in this variable.
 */
static volatile uint8_t currentOutput;

void dali_disable_write() {
    // This should have already been done by the stop bits, but just to be sure.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc | TCA_SINGLE_CMP2EN_bm;   
    // We're done! Disable the timer. 
    TCA0.SINGLE.CTRLA = 0; 
    TCA0.SINGLE.CTRLC = 0; 
    TCA0.SINGLE.INTCTRL = 0;
    dali_transmitting = false;
}



/**
 * This is set up to go off just before a transition from low to high (inverted in real output), and is used to sample
 * the line before toggling, to ensure that there aren't any collisions.
 * 
 */
ISR(TCA0_CMP1_vect) {
    if (dali_read_bus()) {
        // The bus is shorted, which indicates that there is a collision.
        dali_disable_write();
        // TODO signal to the user that its gone wrong!
        log_info("detected collision");
    }
    // Clear interrupt.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP1_bm;
}


/**
 * Called after each time the timer flips the output.  We update the time for the next flip, and check
 * to see when we're finished.
 */
ISR(TCA0_CMP2_vect) {
    uint16_t newPeriod = *pulsePtr++;
    currentOutput = !currentOutput;

    if (newPeriod) {
        // Update both CMP0 and CMP2 to the new waveform period.
        TCA0.SINGLE.CMP2 = TCA0.SINGLE.CMP0 = newPeriod;

        // If we're transmitting the stop bits, we don't want to toggle any more.
        if (newPeriod >= USEC_TO_TICKS(DALI_STOP_BITS_USECS)) {
            // Disable waveform generation, so it doesn't toggle when we're done.  When we release it, it will go back to whatever the port says it is.
            TCA0.SINGLE.CTRLC = 0; 
            TCA0.SINGLE.CTRLB   = TCA_SINGLE_WGMODE_NORMAL_gc | TCA_SINGLE_CMP2EN_bm;   
        }

        if (currentOutput == 0) {
            // We want to check for collision 10 microseconds before we flip it anyway.
            TCA0.SINGLE.CMP1 = newPeriod - USEC_TO_TICKS(10);
        } else {
            // We set CMP1 to a higher value than CMP0, so it will never fire.
            TCA0.SINGLE.CMP1 = newPeriod + 100;
        }
    } else {
        // Done!
        dali_disable_write();
    }
    // Clear interrupt flag.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP2_bm;
}

ISR(AC0_AC_vect) {
    // Clear the interrupt flag
    AC0_STATUS = AC_CMP_bm;
}




void dali_wait_for_transmission() {
    while (dali_transmitting) {
        // TODO make it sleep
        ; // Do nothing. 
    }
}


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
    VREF.CTRLA = VREF_DAC0REFSEL_0V55_gc;
    AC0_MUXCTRLA = AC_MUXPOS_PIN0_gc | AC_MUXNEG_VREF_gc;
    AC0_CTRLA = AC_INTMODE_NEGEDGE_gc | AC_HYSMODE_10mV_gc | AC_ENABLE_bm;



    // Ensure the timer is stopped;
    TCA0.SINGLE.CTRLA = 0;     

    // Set up timer TCA0 for DALI transmission. 
    TCA0.SINGLE.EVCTRL  = 0;

    // Make the TCA.CMP1 interrupt the highest priority, so that it goes off ASAP after the timer goes off.
}


dali_result_t dali_transmit_cmd(uint8_t addr, uint8_t cmd) {

    if (dali_transmitting) {
        return DALI_ALREADY_TRANSMITTING;
    }


    // Check to see that the bus is high (somebody else transmitting), TODO and hasn't been for some time 
    if (dali_read_bus()) {
        return DALI_COLLISION_DETECTED;        
    }


    pulsePtr = pulsePeriods;
    // Reset the clock.
    TCA0.SINGLE.CNT  = 0;

    // Before we start messing with stuff, make sure the timer is off
    TCA0.SINGLE.CTRLA = 0;    
    // Turn on Output Waveform Generation on WO2 only
    TCA0.SINGLE.CTRLC = TCA_SINGLE_CMP2OV_bm;    
    // Initial half bit is always a half low pulse (followed by a half high pulse).
    // We take a bit off the initial timer, to allow for timer startup delay.  This was eyeballed with a logic analyser
    TCA0.SINGLE.CMP2 = TCA0.SINGLE.CMP0 = USEC_TO_TICKS(DALI_HALF_BIT_USECS) - 5;
    TCA0.SINGLE.CMP1 = TCA0.SINGLE.CMP0 + 100; // Effectively disabled.
    // Interrupt on CMP1 for collision detect, CMP2 to update the period
    TCA0.SINGLE.INTCTRL  = TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm;
 
    uint8_t lastBit = 1;
    uint16_t buf = addr << 8 | cmd;
    
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t bit = buf & 0x8000 ? 1 : 0;

        if (bit == lastBit) {
            // Two short pulses.
            *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
            *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
        } else {
            // One long pulse
            *pulsePtr++ = USEC_TO_TICKS(DALI_BIT_USECS);
        }
        lastBit = bit;
        buf <<= 1;
    }
    // We're half way through the last bit.  
    if (lastBit == 0) {
        // If the previous bit was a 0, we need one more toggle to go back to idle.
        *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS);

        // And add the stop bits
        *pulsePtr++ = USEC_TO_TICKS(DALI_STOP_BITS_USECS);
    } else {
        // We're already idle, but we need to wait for stop bits and the last half bit. 
        *pulsePtr++ = USEC_TO_TICKS(DALI_STOP_BITS_USECS + DALI_HALF_BIT_USECS);
    }
    
    // Put sentinel value at the end.
    *pulsePtr++ = 0;
    pulsePtr = pulsePeriods;

    dali_transmitting = true;
    // Set off the first half of the start bit by dragging the output low and turning on the timer.
    // PORTB.OUTSET = PIN2_bm;
    // Put us in Frequency Mode, and enabling CMP2
    currentOutput = 1;

    // Clear out any existing interrupts
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP1_bm;

    // This will immediately drive the output signal high
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc | TCA_SINGLE_CMP2EN_bm;
    // Now start the timer.
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; 

    return DALI_OK;
}