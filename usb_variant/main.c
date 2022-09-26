#define __DELAY_BACKWARD_COMPATIBLE__

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

#define DALI_BAUD                   (1200)
#define DALI_BIT_USECS              (1000000.0/DALI_BAUD)
#define DALI_HALF_BIT_USECS         (DALI_BIT_USECS/2.0)
#define USEC_TO_TICKS(u)    ((uint16_t) (((float)u)*(F_CPU/1000000.0) + 0.5))


typedef enum {
    OK,
    NO_OP,
    BAD_INPUT,
    FAIL,
    BUSY,
    FULL,
} command_response_t;



typedef void (*callback_t)(command_response_t result);

// give it enough space to store a _lot_ of pulses - We only need a max of 50 for a normal 24 bit sequence.
#define MAX_PULSES 60

// static callback_t pinchangeCallback = NULL;
static uint16_t pulses[MAX_PULSES];
static volatile uint16_t *pulsePtr = pulses;
static bool transmitting = false;
static callback_t onCompletionCallback = NULL;

static void sendCommandResponse();


static inline uint8_t isShorted() {
    return PORTA.IN;
}


static inline void resetPulseArray() {
    pulsePtr = pulses;
    pulses[0] = 0;

}

ISR(TCA0_CMP0_vect) {
    uint16_t newPulseWidth = *pulsePtr++;
    if (newPulseWidth == 0 || pulsePtr - pulses == 52) {
        // TCA0.SINGLE.CTRLC = 0; 
        TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc | TCA_SINGLE_CMP2EN_bm;   
        TCA0.SINGLE.CTRLA = 0; // We're done. 
        TCA0.SINGLE.INTCTRL = 0; // Turn off interrupts.

        resetPulseArray();
        // TODO re-enable receiving.
        transmitting = false;
        if (onCompletionCallback) {
            onCompletionCallback(OK);
        }

    } else {
        TCA0.SINGLE.CMP0 = newPulseWidth;
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}

void transmit(callback_t callback) {
    pulsePtr = pulses;
    if (*pulsePtr == 0) {
        callback(NO_OP);
        return; // Degenerate do nothing use case.
    }
    onCompletionCallback = callback;
    // TODO pause receiving
    transmitting = true;

    TCA0.SINGLE.CTRLA = 0;     // Disable timer if it was already running
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm; // Clear any existing interrupts.
    TCA0.SINGLE.CNT  = 0;
    uint16_t firstPulse = *pulsePtr++;
    TCA0.SINGLE.CMP0 = firstPulse; // Initial pulse width.
    TCA0.SINGLE.CMP2 = 0; // Toggle immediately.  It will always be out of phase with WG0 
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;  // Now start the timer.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc | TCA_SINGLE_CMP2EN_bm; // This will immediately drive the output signal high (shorted)
    TCA0.SINGLE.INTCTRL  = TCA_SINGLE_CMP0_bm; // Interrupt on CMP2 to update the period
}

static inline bool isPulseArrayFull() {
    return pulsePtr - pulses == MAX_PULSES;
}

static uint8_t getPulseCount() {
    return pulsePtr - pulses;
}

void transmitValidDali(uint32_t data, uint8_t numBits, callback_t callback) {
    pulsePtr = pulses;

    *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS); // First half of start bit.
    data = data << (32-numBits);  // Add in start bit
    uint8_t last = 1;

    for (uint8_t i = 0; i < numBits; i++) {
        uint8_t current = data & 0x80000000 ? 1 : 0;

        if (current == last) {
            *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS); // Two toggles
            *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS); 
        } else {
            *pulsePtr++ = USEC_TO_TICKS(DALI_BIT_USECS); // One big long one 
        }
        last = current;
        data <<=1;
    }
    // At this point we're half way through the last bit, but we only care if the final bit was a 0, as that means the bus
    // has just been shorted.  We want to release after another half bit.
    if (!last) {
        // Need one last half bit flip. 
        *pulsePtr++ = USEC_TO_TICKS(DALI_HALF_BIT_USECS);
    }
    *pulsePtr = 0; //sentinel value. 
    if (getPulseCount() % 2 == 0) {
        log_info("Error - expected odd number of pulse durations");
        callback(FAIL);
    } else {
        transmit(callback);
    }
}

static void sendCommandResponse(command_response_t response) {
    switch (response) {
        case OK: 
            printf("\r\n<OK\r\n");        
            break;
        case NO_OP: 
            printf("\r\n<NOOP\r\n");        
            break;
        case BAD_INPUT: 
            printf("\r\n<!Bad Input\r\n");        
            break;
        case FAIL: 
            printf("\r\n<!Failed\r\n");        
            break;
        case FULL: 
            printf("\r\n<!Full\r\n");        
            break;
        case BUSY:
            printf("\r\n<!Busy\r\n");        
            break;
    }
}

static void command_parser(char *cmd) {
    uint8_t len = strlen(cmd);
    uint16_t pw;

    if (len == 0) {
        // Empty command. Do nothing!
    } else {
        switch (cmd[0]) {
            // Transmit a valid DALI command
            case 'T':
                // We only accept 16 and 24 bit payloads for T
                if (!(len == 5 || len == 7)) {
                    sendCommandResponse(BAD_INPUT);
                    return;
                }
                if (transmitting) {
                    sendCommandResponse(BUSY);
                    return;
                }
                uint32_t val = strtol(cmd+1, NULL, 16);
                transmitValidDali(val, (len-1)*4, sendCommandResponse);
                return;

            // Clear out any accumulated pulses. 
            case 'C': 
                resetPulseArray();
                sendCommandResponse(OK);
                return;

            // append a pulse.
            case 'P': 
                pw = atoi(cmd+1)*10/3;
                // Minimum pulsewidth is 10 uSec
                if (pw < 34) {
                    sendCommandResponse(BAD_INPUT);
                    return;
                }
                if (isPulseArrayFull()) {
                    sendCommandResponse(FULL);
                    return;
                }

                *pulsePtr++ = pw;
                *pulsePtr = 0; // Ensure that pulse array always ends with a 0.
                log_info("Added pulse of %d ticks", pw);
                sendCommandResponse(OK);
                return;

            // Send our accumulated waveform. 
            case 'W': 
                if (getPulseCount() % 2 == 0) {
                    log_info("Error - expected odd number of pulse durations");
                    sendCommandResponse(BAD_INPUT);
                    return;
                }
            
                transmit(sendCommandResponse);
                return;
        }
    }
    sendCommandResponse(BAD_INPUT);
}



int main(void) {
    console_init(command_parser);

    sei();

    log_info("Welcome");

    // Set PB2 as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Set PA7  as in input, with pullup.
    // PORTA.PIN7CTRL = PORT_PULLUPEN_bm;
    PORTA.DIRCLR = PORT_INT7_bm;


    CPUINT.LVL1VEC = TCA0_CMP0_vect_num; // Make CMP0 high priority.

    while (1) {
        _delay_ms(500);
    }
    return 0;
}
