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
#define DALI_MARGIN_USECS           (45)
#define USEC_TO_TICKS(u)    ((uint16_t) (((float)u)*(F_CPU/1000000.0) + 0.5))
#define TICKS_TO_USECS(u)    ((u)/(F_CPU/1000000.0))




typedef void (*callback_t)(command_response_t result);

// give it enough space to store a _lot_ of pulses - We only need a max of 50 for a normal 24 bit sequence.
#define MAX_PULSES 60

// static callback_t pinchangeCallback = NULL;
static uint16_t pulses[MAX_PULSES];
static volatile uint16_t *pulsePtr = pulses;
static bool transmitting = false;
static callback_t onCompletionCallback = NULL;
static volatile bool readStateWaiting = true;

static void enableRead();
static void disableRead();


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
        // Finished.  Turn off the timer. 
        // TCA0.SINGLE.CTRLC = 0; 
        TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc | TCA_SINGLE_CMP2EN_bm;   
        TCA0.SINGLE.CTRLA = 0; // We're done. 
        TCA0.SINGLE.INTCTRL = 0; // Turn off interrupts.

        resetPulseArray();
        transmitting = false;
        if (onCompletionCallback) {
            onCompletionCallback(CMD_OK);
        }
        enableRead();
    } else {
        TCA0.SINGLE.CMP0 = newPulseWidth;
    }
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}

void transmit(callback_t callback) {
    pulsePtr = pulses;
    if (*pulsePtr == 0) {
        callback(CMD_NO_OP);
        return; // Degenerate do nothing use case.
    }
    onCompletionCallback = callback;
    disableRead();
    transmitting = true;

    TCA0.SINGLE.CTRLA = 0;     // Disable timer if it was already running
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm | TCA_SINGLE_OVF_bm; // Clear any existing interrupts.
    TCA0.SINGLE.CNT  = 0;
    uint16_t firstPulse = *pulsePtr++;
    TCA0.SINGLE.CMP0 = firstPulse; // Initial pulse width.
    TCA0.SINGLE.CMP2 = 0; // Toggle immediately.  It will always be out of phase with WG0 
    TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;  // Now start the timer.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_FRQ_gc | TCA_SINGLE_CMP2EN_bm; // This will immediately drive the output signal high (shorted)
    TCA0.SINGLE.INTCTRL  = TCA_SINGLE_CMP0_bm; // Interrupt on CMP0 to update the period
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
        callback(CMD_FAIL);
    } else {
        transmit(callback);
    }
}

static void outputPulses() {
    bool first = true;
    uint16_t val;

    pulsePtr = pulses;
    printf("\rI");
    while ((val = *pulsePtr++)) {
        if (first) {
            first = false;
        } else {
            putchar(',');
        }
        printf("%d", (uint16_t) TICKS_TO_USECS(val));
    }
    printf("\r\n");
    printPrompt();
    resetPulseArray();
}


static command_response_t command_parser(char *cmd) {
    uint8_t len = strlen(cmd);
    uint16_t pw;
    char *endptr;

    if (len == 0) {
        // Empty command. Do nothing!
        return CMD_NO_OP;
    } else {
        switch (cmd[0]) {
            // Transmit a valid DALI command
            case 'T':
                // We only accept 16 and 24 bit payloads for T
                if (!(len == 5 || len == 7)) {
                    return CMD_BAD_INPUT;
                }
                if (transmitting) {
                    return CMD_BUSY;
                }
                uint32_t val = strtol(cmd+1, &endptr, 16);
                if (*endptr) {
                    // We found an invalid character
                    return CMD_BAD_INPUT;
                }
                transmitValidDali(val, (len-1)*4, sendCommandResponse);
                return CMD_DEFERRED_RESPONSE;

            // Clear out any accumulated pulses. 
            case 'C': 
                resetPulseArray();
                return CMD_OK;

            // append a pulse.
            case 'P': 
                pw = atoi(cmd+1)*10/3;
                // Minimum pulsewidth is 10 uSec
                if (pw < 34) {
                    return CMD_BAD_INPUT;
                }
                if (isPulseArrayFull()) {
                    return CMD_FULL;
                }

                *pulsePtr++ = pw;
                *pulsePtr = 0; // Ensure that pulse array always ends with a 0.
                log_info("Added pulse of %d ticks", pw*3/10);
                return CMD_OK;

            // Send our accumulated waveform. 
            case 'W': 
                if (getPulseCount() % 2 == 0) {
                    log_info("Error - expected odd number of pulse durations");
                    return CMD_BAD_INPUT;
                }            
                transmit(sendCommandResponse);
                return CMD_DEFERRED_RESPONSE;
        }
    }
    return CMD_BAD_INPUT;
}


static void setupTimeout(uint16_t timeout) {
    TCA0.SINGLE.CTRLA = 0;
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;  // Clear any existing interrupt. 
    TCA0.SINGLE.CNT = 0;  // We try to always reset timer counters, but make sure. 
    TCA0.SINGLE.PER = timeout; // Send ISR when clock reaches PER.
    TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; // Enable OVF interrupt.
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // normal counter mode, no output.
}


// We allow 45 uSec variation in timing (371 - 461)
static inline bool isHalfBit(uint16_t v) {
    return v >= USEC_TO_TICKS(DALI_HALF_BIT_USECS-DALI_MARGIN_USECS) && v <= USEC_TO_TICKS(DALI_HALF_BIT_USECS+DALI_MARGIN_USECS);
}

// We allow 45 uSec variation in timing (788 - 878)
static inline bool isFullBit(uint16_t v) {
    return v >= USEC_TO_TICKS(DALI_BIT_USECS-DALI_MARGIN_USECS) && v <= USEC_TO_TICKS(DALI_BIT_USECS+DALI_MARGIN_USECS);
}



static bool decodeDaliInput(uint32_t *out, uint8_t *numBits) {
    pulsePtr = pulses;
    uint8_t last;
    uint16_t currentPulseWidth;

    // First pulse must be a half bit.
    if (!isHalfBit(*pulsePtr++)) {
        return false;
    }
    last = 1;

    while ((currentPulseWidth = *pulsePtr++)) {
        // We should be at the half bit mark of the previous bit at this stage.
        if (isFullBit(currentPulseWidth)) {
            // next bit is the opposite polarity to the previous.
            last ^= 1;
        } else if (isHalfBit(currentPulseWidth)) {
            // Its the same bit, and we expect another half bit pulse immediately after. 
            if (!isHalfBit(*pulsePtr++)) {
                return false;
            }
        } else {
            // Invalid pulse width 
            return false;
        }
        *out = *out << 1 | last;
        (*numBits)++;
    }
    return true;
    

}


ISR(TCA0_OVF_vect) {
    // A timeout occurred
    TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear flag.
    TCA0.SINGLE.CTRLA = 0; // Turn off timer.

    if (AC0.STATUS & AC_STATE_bm) {
        // Timeout occurred while shorted.  This shouldn't happen.
        log_info("Timeout while shorted");
    }
    
    uint32_t val = 0;
    uint8_t numBits = 0;
    if (decodeDaliInput(&val, &numBits) && (numBits % 8) == 0) {
        printf("\rD");
        while (numBits > 0) {
            numBits -= 8;
            printf("%02x", (unsigned int) (val >> numBits) & 0xFF);
        }
        printf("\r\n");
        printPrompt();
        resetPulseArray();
    } else {
        outputPulses();
    }

    // Return TCB0 to its initial state
    TCB0.EVCTRL = TCB_CAPTEI_bm; // Waiting for a positive edge to start with.
    readStateWaiting = true;

}




ISR(TCB0_INT_vect) {
    uint16_t cnt = TCB0.CCMP; // This will clear the interrupt flag.
    TCB0.EVCTRL ^= TCB_EDGE_bm;  // Toggle the edge we're looking for. 

    if (readStateWaiting) {
        // This is the initial low -> high transition.
        readStateWaiting = false;
        pulsePtr = pulses;
        *pulsePtr = 0;
        
        // By definition we should be high (shorted) right now, so no need to set up a timeout.
    } else {
        if (!isPulseArrayFull()) {
            *pulsePtr++ = cnt;
            *pulsePtr = 0;
        }

    }
    // Start a timer that will go off after the maximum wait time (2 Bit periods) to indicate we're done.
    TCA0.SINGLE.CNT = 0; // Reset the timout clock
    TCA0.SINGLE.CTRLA =  TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm; // Make the timeout clock run

}


static void enableRead() {
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm; // Waiting for a positive edge to start with.
    TCB0.INTCTRL = TCB_CAPT_bm; // Enable Interrupts on CAPTURE
    TCB0.INTFLAGS = TCB_CAPT_bm; // Clear any existing interrupt.

    readStateWaiting = true;
    setupTimeout(USEC_TO_TICKS(2*DALI_BIT_USECS));
    // if (timeout) {
    //     on_timeout(timeout, callback);
    //     TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;  // Start timeout timer.
    // }
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // Start TCB0 for pulse width timing - First pulse should reset clock.
}

static void disableRead() {
    TCB0.EVCTRL = 0; // Disable Event handling
    TCA0.SINGLE.CTRLA = 0;  // Stop timeout timer.
    TCB0.CTRLA = 0; // Stop TCB0
    TCB0.INTFLAGS = TCB_CAPT_bm; // Clear any interrupts.
}



int main(void) {
    console_init(command_parser);

    // Set PB2 as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Set PA7 as in input, with external pullup (to make it as strong as possible).
    // PORTA.DIRCLR = PORT_INT7_bm;

    // Use PA7 as an Analog Comparator, with reference of 0.55V
    // This makes it trigger sooner than if we were doing digital I/O
    VREF.CTRLA = VREF_DAC0REFSEL_0V55_gc;
    PORTA.PIN7CTRL  = PORT_ISC_INPUT_DISABLE_gc; // Disable Digital I/O
    AC0.MUXCTRLA = AC_MUXNEG_VREF_gc | AC_MUXPOS_PIN0_gc;
    AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm;
    EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_AC0_OUT_gc;
    EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc; // Set TCB0 to use ASYNCHCH0



    
    // EVSYS.SYNCCH0 = EVSYS_SYNCCH0_PORTA_PIN7_gc;  // Route Pin7 to put out events.
    // EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_SYNCCH0_gc; // Set TCB0 to use SYNCHCH0
        
    // // CPUINT.LVL1VEC = TCA0_CMP0_vect_num; // Make CMP0 high priority.

    enableRead();
    sei();

    
    
    while (1) {
        _delay_ms(500);
    }
    return 0;
}
