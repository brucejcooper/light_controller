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

volatile bool collision_detected = false;

static volatile uint16_t shiftreg = 0;
static volatile uint8_t half_bits_remaining = 0;
static volatile uint8_t next_output;



ISR(PORTA_PORT_vect)
{
    if(DALI_PORT.INTFLAGS & DALI_RX_bm) {

        // This is an indication that there was a falling edge during a high period of the bus while writing.
        // i.e. somebody else drew the line low when it should be us in control.  A collision has occurred.
        // Disable our interrupts, this detector, and set the collision_detected flag to be true.
        TCA0.SINGLE.CTRLA = 0;
        DALI_PORT.PIN3CTRL = PORT_PULLUPEN_bm; 
        DALI_PORT.OUTSET = DALI_TX_bm;
        half_bits_remaining = 0;
        collision_detected = true;

        // Clear flag
        DALI_PORT.INTFLAGS = DALI_RX_bm;          
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
    TCA0.SINGLE.PER = 1386; // at 3.3Mhz, this will go off every 416.4us)
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; // Enable interrupt on overflow.
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


static dali_result_t read_bit_into_lsb(uint16_t *out) {
    // Read initial value
    uint8_t val = DALI_PORT.IN & DALI_RX_bm;

    // Wait for transition to opposite value for half bit +/- 10%
    // wait for transition (expecting none) for half bit +/- 10%
}



static dali_result_t read_start_bit() {
    uint16_t tmp;
    dali_result_t res = read_bit_into_lsb(&tmp);

    if (res == DALI_OK && tmp & 0x01) {
        return DALI_OK;
    }
}


#define TICKS_TO_USEC(t)    (t/(F_CPU/1000000.0))

/**
 * When receiving. Set up edge transition interrupts.  Run the timer TCA0 along side it.  Record how many uSecs it spends in each state
 * 
 * start bit should be 416 +/- 10% low then either 416 high or 833 high... and so on and so forth.
 * 
 * In the event of any failures, wait until you get at least 2ms of empty bus.
 */


dali_result_t dali_receive(uint8_t *address, uint8_t *command) {
    // If the bus is high, then its not the start of a packet.
    if (DALI_PORT.IN & DALI_RX_bm) {
        return DALI_NO_START_BIT;
    }



    dali_result_t res = DALI_OK;
    res = read_start_bit();
    uint16_t packet = 0;
    for (int i = 0; res == DALI_OK && i < 16; i++) {
        packet <<= 1;
        res = read_bit_into_lsb(&packet);
    }
    return res;
}
