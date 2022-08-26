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
static volatile bool transmitting = false;



ISR(TCA0_CMP2_vect) {
    uint16_t newPeriod = *pulsePtr++;

    if (newPeriod) {
        // Update both CMP0 and CMP2 to the new waveform period.
        TCA0.SINGLE.CMP2 = TCA0.SINGLE.CMP0 = newPeriod;

        // If we're transmitting the stop bits, we don't want to toggle any more.
        if (newPeriod >= USEC_TO_TICKS(DALI_STOP_BITS_USECS)) {
            // Disable waveform generation, so it doesn't toggle when we're done.  When we release it, it will go back to whatever the port says it is.
            TCA0.SINGLE.CTRLC = 0; 
            TCA0.SINGLE.CTRLB   = TCA_SINGLE_WGMODE_NORMAL_gc | TCA_SINGLE_CMP2EN_bm;   
        }
    } else {
        // We're done! Disable the timer. 
        TCA0.SINGLE.CTRLA = 0; 
        transmitting = false;
    }
    // Clear interrupt flag.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP2_bm;
}

void dali_wait_for_transmission() {
    while (transmitting) {
        ; // Do nothing.
    }
}


void dali_init() {

    // Output is PA2 (WO2) - Initially low (which will be inverted by the transistor) 
    // whenever we're not waveform generating, the pin will return to this level.
    PORTB.OUTCLR = PIN2_bm;
    PORTB.DIRSET = PIN2_bm;

    // Ensure the timer is stopped;
    TCA0.SINGLE.CTRLA = 0;     

    // Set up timer TCA0 for DALI transmission.  Interrupt on CMP2
    TCA0.SINGLE.EVCTRL  = 0;
    TCA0.SINGLE.INTCTRL  = TCA_SINGLE_CMP2_bm;
}



dali_result_t dali_transmit_cmd(uint8_t addr, uint8_t cmd) {

    if (transmitting) {
        return DALI_ALREADY_TRANSMITTING;
    }

    // TODO Check to see that the bus is high (not transmitting), and hasn't been for some time 


    pulsePtr = pulsePeriods;
    // Reset the clock.
    TCA0.SINGLE.CNT  = 0;

    // BEfore we start messing with stuff, make sure the timer is off
    TCA0.SINGLE.CTRLA = 0;    

    // Turn on Output Waveform Generation on WO2 only
    TCA0.SINGLE.CTRLC = TCA_SINGLE_CMP2OV_bm;    
    // Initial half bit is always a half low pulse (followed by a half high pulse).
    TCA0.SINGLE.CMP2 = TCA0.SINGLE.CMP0 = USEC_TO_TICKS(DALI_HALF_BIT_USECS) - 345;
    // Put us in Frequency Mode, and enabling CMP2
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc | TCA_SINGLE_CMP2EN_bm;


    
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

    transmitting = true;
    // Set off the first half of the start bit by dragging the output low and turning on the timer.
    // PORTB.OUTSET = PIN2_bm;
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; 

    return DALI_OK;
}