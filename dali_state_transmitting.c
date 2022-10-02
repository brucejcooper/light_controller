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


// At maximum, we need two pulses for each bit (max 24x2), pluse one extra for the final clean up, plus one stop half bit, plus stop bits.

static volatile uint32_t shiftReg;
static volatile int8_t halfBitsLeft; // Signed because we allow this to go negative.
static volatile uint8_t lastBit;
static dali_transmit_completed_callback_t completionCallback;


/**
 * Output is driven by the waveform generator built into TCA0.  This method restores the timer mode to 
 * normal, which will revert to driving the output based on GPIO, which should be released. 
 */
static inline void disable_output() {
    // Change TCA0 to be back to normal - Output will revert to High value.
    TCA0.SINGLE.CTRLC = 0; 
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc | TCA_SINGLE_CMP2EN_bm;   
}

static inline void cleanup() {
    TCA0.SINGLE.CTRLA = 0; 
    // This should have already been done by the stop bits, but just to be sure.
    disable_output();
    TCA0.SINGLE.INTCTRL = 0; // Turn off all interrupts.
    // We're done! Disable the timer. 
    TCA0.SINGLE.CNT = 0;
}



/**
 * This is set up to go off just before a transition from low to high (inverted in real output), and is used to sample
 * the line before toggling, to ensure that there aren't any collisions.
 * 
 */
ISR(TCA0_CMP1_vect) {
    if (dali_is_bus_shorted()) {
        // that there is a collision. go into failsafe mode.
        cleanup();
        completionCallback(true);
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP1_bm;
}


/**
 * Called after each time the timer flips the output.  We update the time for the next flip, and check
 * to see when we're finished.
 */
ISR(TCA0_CMP2_vect) {
    uint16_t newPeriod = 0;
    bool collisionDetect;

    if (halfBitsLeft > 0) {
        if (halfBitsLeft % 2 == 0) {
            // We're at the half way point of the previous bit - determine how long the next flip will be. 
            uint8_t nextBit = shiftReg & 0x80000000L ? 1 : 0;
            shiftReg <<= 1;
            if (nextBit == lastBit) {
                // Next bit is the same as the current bit. 
                halfBitsLeft--; // By decrementing it by 1, we make it odd, and it will do two flips of the same length.
                newPeriod = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
            } else {
                // Its a change in bit
                halfBitsLeft -= 2;
                newPeriod = USEC_TO_TICKS(DALI_BIT_USECS);
                lastBit = nextBit;
            }
            collisionDetect = lastBit == 1;
        } else {
            // Now at bit edge.  Simply wait another half bit. 
            newPeriod =  USEC_TO_TICKS(DALI_HALF_BIT_USECS);
            collisionDetect = lastBit == 0;
            halfBitsLeft--;
        }
    } else if (halfBitsLeft == 0) {
        // We're now half way through the last bit. 
        if (lastBit == 0) {
            // If our last bit was a 0, we need one last low value.
            newPeriod =  USEC_TO_TICKS(DALI_HALF_BIT_USECS);
            halfBitsLeft--;
            collisionDetect = false;
        } else {
            // Our output is already idle, so we just need to reset to idle and output stop (idle) for 2.5 Bit periods.
            halfBitsLeft -= 2;
            collisionDetect = true;
            disable_output();
            newPeriod =  USEC_TO_TICKS(DALI_BIT_USECS*2 + DALI_HALF_BIT_USECS);
        }
        
    } else if (halfBitsLeft == -1) {
        // We are now at the end of the last 0 bit, so we reset to idle and output stop bits for 2 Bit period.
        disable_output();
        collisionDetect = true;
        newPeriod =  USEC_TO_TICKS(DALI_BIT_USECS*2);
    } else {
        // Transmit Complete.
        cleanup();
        collisionDetect = false;
        completionCallback(false);
    }
    if (newPeriod) {
        TCA0.SINGLE.CMP0 = TCA0.SINGLE.CMP2 = newPeriod;
        TCA0.SINGLE.CMP1 =/* collisionDetect ? newPeriod - USEC_TO_TICKS(8) :*/ newPeriod + USEC_TO_TICKS(8);
    }
    // Clear interrupt flag.
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP2_bm;
}



extern void dali_transmitting_state_enter(uint32_t data, uint8_t numBits, dali_transmit_completed_callback_t callback) {
    log_info("tx");
    completionCallback = callback;

    // Set up timer A to do output etc...  Set up buffer with timings.  Start Timer A. 
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

    // Also turn on AC0 but no interrupts, as we are sampling.
    dali_on_linechange(NULL); 

    shiftReg = data << (32-numBits);
    halfBitsLeft = (int8_t) numBits * 2;    
    // Set off the first half of the start bit by dragging the output low and turning on the timer.
    // Put us in Frequency Mode, and enabling CMP2
    lastBit = 1;

    // Clear out any existing interrupts
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm;
    // This will immediately drive the output signal high (shorted)
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc | TCA_SINGLE_CMP2EN_bm;
    // Now start the timer.
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; 
}

