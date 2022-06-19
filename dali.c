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

// Good documentation for Dali commands - https://www.nxp.com/files-static/microcontrollers/doc/ref_manual/DRM004.pdf

// The DALI configuration - PA4 for transmit - PA3 for receive.
#define DALI_TX_bm      PIN4_bm
#define DALI_RX_bm      PIN3_bm
#define DALI_PORT       PORTA


typedef enum  {
    ED_COLLISION,
    ED_READ,
} edgedetect_mode_t;

volatile bool collision_detected = false;

static volatile uint16_t shiftreg = 0;
static volatile uint8_t half_bits_remaining = 0;
static volatile uint8_t next_output;
static volatile edgedetect_mode_t edgedetect_mode = ED_COLLISION;


ISR(PORTA_PORT_vect)
{
    if(DALI_PORT.INTFLAGS & DALI_RX_bm) {
        if (edgedetect_mode == ED_COLLISION) {
            // This is an indication that there was a falling edge during a high period of the bus while writing.
            // i.e. somebody else drew the line low when it should be us in control.  A collision has occurred.
            // Disable our interrupts, this detector, and set the collision_detected flag to be true.
            TCA0.SINGLE.CTRLA = 0;
            DALI_PORT.PIN3CTRL = PORT_PULLUPEN_bm; 
            DALI_PORT.OUTSET = DALI_TX_bm;
            half_bits_remaining = 0;
            collision_detected = true;
        }

        // Clear flag
        DALI_PORT.INTFLAGS &= DALI_RX_bm;          
    }
}


ISR(TCA0_OVF_vect)
{
    // while transmitting, this is called every 416.4us
    if (next_output) {
        DALI_PORT.OUTSET = DALI_TX_bm;
        // Turn on falling edge detection on RX line (collision detection)
        DALI_PORT.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
    } else {
        // Turn off falling edge collision detection on RX line, as we are driving it low ourselves
        DALI_PORT.PIN3CTRL = PORT_PULLUPEN_bm; 
        DALI_PORT.OUTCLR = DALI_TX_bm;
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear flag.
    if (--half_bits_remaining > 0) {
        if ((half_bits_remaining % 2) == 1) {
            next_output = shiftreg & (0x01 << 15) ? 0 : 1; // The MSB.
            shiftreg <<= 1;
        } else {
            // We're half way through the bit, so toggle.
            next_output = !next_output;
        }
    } else {
        // We're done. Disable the timer (Remove the enable bit from CTRLA)
        TCA0.SINGLE.CTRLA = 0;
        // Return the bus high for stop bits.
        DALI_PORT.OUTSET = DALI_TX_bm;
    }
}


void dali_init() {
    // Set up dali TX as an output (initially high)
    DALI_PORT.OUTSET = DALI_TX_bm;
    DALI_PORT.DIRSET = DALI_TX_bm;
    // and RX as an Input, with pullup.
    DALI_PORT.DIRCLR = DALI_RX_bm;
    DALI_PORT.PIN3CTRL = PORT_PULLUPEN_bm;
    // DRive the dali bus High.
    DALI_PORT.OUTSET = DALI_TX_bm;

    // Set up timer TCA0 for DALI transmission
    TCA0.SINGLE.EVCTRL  = 0;
    TCA0.SINGLE.CTRLB   = 0x00;

    // Make our timer interrupt higher priority than other interrupts.
    CPUINT.LVL1VEC = TCA0_OVF_vect_num;

}


dali_result_t dali_transmit_cmd(uint8_t addr, uint8_t cmd) {
    shiftreg = addr << 8 | cmd;
    half_bits_remaining = (16+1)*2;
    // Clear collision detected flag.
    collision_detected = false;


    TCA0.SINGLE.PER = 1386; // at 3.3Mhz, this will go off every 416.4us)
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; // Enable interrupt on overflow.

    TCA0.SINGLE.CNT = 18; // Set the timer to be a bit quicker for the first half bit, to account for additional statements, and the overhead of the ISR.
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; // Enable the timer.
    // Process first half of start bit (drive low), and set up next half-bit to be high.
    DALI_PORT.OUTCLR = DALI_TX_bm;
    next_output = 1;


    // Wait for the bits to be transmitted.  We know we are finished when the timer is disabled.
    while (half_bits_remaining > 0) {
        ;
    }
    // Wait for the stop bits - send line high (done by timer), then wait for 2 bit periods (4 half bits)
    _delay_us(833*2);
    // Wikipedia says you need at east 2.45ms of idle..

    
    if (collision_detected) {
        return DALI_COLLISION_DETECTED;
    } else {
        return DALI_OK;
    }
}

// Only correct for clock of 3.33Mhz
#define TICKS_TO_USEC(t)    (t*3/10)
#define USEC_TO_TICKS(u)    (u*10/3)

#define HALFTICK_MIN_TICKS USEC_TO_TICKS(413-42)
#define HALFTICK_MAX_TICKS USEC_TO_TICKS(413+42)
#define FULLTICK_MIN_TICKS USEC_TO_TICKS(833-42)
#define FULLTICK_MAX_TICKS USEC_TO_TICKS(833+42)

/**
 * When receiving. Set up edge transition interrupts.  Run the timer TCA0 along side it.  Record how many uSecs it spends in each state
 * 
 * start bit should be 416 +/- 10% low then either 416 high or 833 high... and so on and so forth.
 * 
 * In the event of any failures, wait until you get at least 2ms of empty bus.
 */

typedef enum {
    PULSE_TIMING,
    PULSE_HALF_BIT,
    PULSE_FULL_BIT,
    PULSE_INVALID_TOO_SHORT,
    PULSE_INVALID_MIDDLE,
    PULSE_INVALID_TOO_LONG,
} pulsewidth_t;

static volatile pulsewidth_t current_pulse_width = PULSE_TIMING;


ISR(TCB0_INT_vect)
{
    uint16_t pulse_length = TCB0.CCMP;  // This will clear the interrupt flag.
    
    if (pulse_length < HALFTICK_MAX_TICKS) {
        current_pulse_width = PULSE_INVALID_TOO_SHORT;
    } else if (pulse_length <= HALFTICK_MAX_TICKS) {
        current_pulse_width = PULSE_HALF_BIT;
    } else if (pulse_length <= FULLTICK_MIN_TICKS) {
        current_pulse_width = PULSE_INVALID_TOO_SHORT;
    } else if (pulse_length <= FULLTICK_MAX_TICKS) {
        current_pulse_width = PULSE_FULL_BIT;
    } else {
        current_pulse_width = PULSE_INVALID_TOO_LONG;
    }

    // Flip the edge type we're looking for.
    TCB0.EVCTRL ^= TCB_EDGE_bm; // 1 is negative edge, 0 is positive edge
}


pulsewidth_t time_pulse(uint8_t *current_val) {
    // Use TCB in Input Capture Frequency Measurement mode.  This will generate a CAPT interrupt on edge transition, and copy
    // the clock value into CCMP.  This will result in accurate pulse width measurement.
    // How to deal with a pulse that goes on for a long high pulse (like a stop condition) though?
    // Have the main loop check to see if CNT goes above the too long threhsold.  When it does, disable the counter and proceed as if TOO_LONG

    
    // is there a race condition here on compare to CNT?  If there is, it'll just loop again, and you'll be fine.
    for (current_pulse_width = PULSE_TIMING; current_pulse_width == PULSE_TIMING && TCB0.CNT < FULLTICK_MAX_TICKS; ) { 
        // We're just waiting, so do nothing.
    }
    pulsewidth_t result = current_pulse_width == PULSE_TIMING ? PULSE_INVALID_TOO_LONG : current_pulse_width;
    current_pulse_width = PULSE_TIMING;
    return result;
}


dali_result_t dali_receive(uint8_t *address, uint8_t *command) {
    dali_result_t res = DALI_CORRUPT_READ;
    uint8_t last_bit = 1;
    uint8_t current_val = 0;
    uint16_t shiftreg = 0;
    pulsewidth_t pulse_width;

    // Check to see if the line has been pulled down (indicating the start of a transmission)
    if (DALI_PORT.IN & DALI_RX_bm) {
        return DALI_NO_START_BIT;
    }

    // We know we're low at this point, which is hopefully the start of a transmission. Set up the timer.
    TCB0.CNT = 3;
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm; // Waiting for a positive edge to start with.
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;


    // Wait for first half bit of the start bit, which must be low for one half bit period.
    pulse_width = time_pulse(&current_val);
    if (pulse_width != PULSE_HALF_BIT) {
        goto cleanup;
    }

    // Now we're half way through the start bit.
    while (res != DALI_OK) {
        // At the top of the loop we are always a half bit period into a bit.
        pulse_width = time_pulse(&current_val);

        if (pulse_width == PULSE_HALF_BIT) {
             // There must be a followup half tick sized pulse (unless line has been high for a while, and the last bit was a zero, indicating an end of transmission)
             pulse_width = time_pulse(&current_val);

             if (pulse_width == PULSE_HALF_BIT) {
                shiftreg = (shiftreg << 1) | last_bit;    
             } else if (pulse_width == PULSE_INVALID_TOO_LONG && current_val > 0 && last_bit == 0) {
                // that was the end of the packet.
                res = DALI_OK;
             }
        } else if (pulse_width == PULSE_FULL_BIT) {
            // Its a bit toggle, and we remain in the half bit position.
            last_bit = last_bit ? 0 : 1;
            shiftreg = (shiftreg << 1) | last_bit;
        } else if (pulse_width == PULSE_INVALID_TOO_LONG && current_val > 0 && last_bit == 1) {
            // The end of the packet.
            res = DALI_OK;
        } else {
            // Its an invalid pulse
            goto cleanup;
        }
    } 

    // If we get here, we've already waited for the line to be high for 1 bit period, but we now need to wait for one more
cleanup:
    TCB0.CTRLA = 0; // Disable TCB0
    // wait for line to be high for at least x ms
    // step 1 - wait for it to be high.
    while ((DALI_PORT.IN & DALI_RX_bm) == 0) {
        // Do nothing.
    }
    _delay_ms(1); // Poor mans way of waiting for line...
    return res;
}
