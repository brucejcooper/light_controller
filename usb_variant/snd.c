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
#include "timing.h"
#include "idle.h"
#include "snd.h"


static uint32_t shiftReg;
static uint8_t bitsLeft;
static uint8_t lastBit;
static void at_half_bit();


static void at_bit_boundary() {
    // Toggle in 1/2 bit to go back to half bit position.
    TCA0.SINGLE.CMP0 = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
    set_output_pulse_handler(at_half_bit);
}

static void cleanup() {
    TCA0.SINGLE.CTRLA = 0;
    TCA0.SINGLE.CTRLB = 0; 
    TCA0.SINGLE.INTCTRL = 0; // Interrupt on CMP0 to update the period
    sendCommandResponse(CMD_OK);
    idle_enter();
}

static void at_half_bit() {
    uint8_t nextBit = (shiftReg & 0x80000000) ? 1 : 0;
    uint16_t newPulseWidth;

    if (bitsLeft == 0) {
        if (lastBit == 0) {
            // Need one last half bit toggle to get it back to idle. 
            newPulseWidth = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
            set_output_pulse_handler(cleanup);
        } else {
            // We're already high (idle) - No need for any more timer loops. 
            cleanup();
            return;
        }
    } else {
        if (nextBit == lastBit) {
            newPulseWidth = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
            set_output_pulse_handler(at_bit_boundary);
        } else {
            lastBit = nextBit;
            newPulseWidth = USEC_TO_TICKS(DALI_BIT_USECS);
            // Stay with the current handler.
        }
        shiftReg <<= 1;
        bitsLeft--;
    }

    TCA0.SINGLE.CMP0 = newPulseWidth;
}


void transmit(uint32_t val, uint8_t len) {
    log_info("Transmitting 0x%08lx(%d)", val, len);


    (shiftReg = val << (32-len));
    bitsLeft = len;
    lastBit = 1;


    // Disable TCB0. 
    TCB0.CTRLA = 0;
    TCB0.CTRLB = 0;


    TCA0.SINGLE.CTRLA = 0;     // Disable timer if it was already running
    TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESET_gc; // Reset the timer - This way it doesn't matter how many toggles there have been. 
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm | TCA_SINGLE_CMP2_bm | TCA_SINGLE_OVF_bm; // Clear any existing interrupts.
    TCA0.SINGLE.INTCTRL  = TCA_SINGLE_CMP0_bm; // Interrupt on CMP0 to update the period
    TCA0.SINGLE.CMP0 = USEC_TO_TICKS(DALI_HALF_BIT_USECS); // We want this to fire pretty much immediately.
    TCA0.SINGLE.CMP2 = 0;
    TCA0.SINGLE.CNT  = 0; // Get timer to fire immediately after starting timer
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc | TCA_SINGLE_CMP2EN_bm; 
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;  // start the timer.
    set_isrs(at_half_bit, NULL, NULL);
}
