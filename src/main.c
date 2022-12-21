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
#include "console.h"
#include "intr.h"
#include "snd.h"
#include "rcv.h"
#include "timing.h"
#include "buttons.h"
#include "state_machine.h"
#include "outgoing_commands.h"
#include "config.h"
#include "timers.h"

static command_response_t command_parser(char *cmd) {
    uint8_t len = strlen(cmd);
    // char *endptr;

    if (len == 0) {
        // Empty command. Do nothing!
        return CMD_NO_OP;
    } else {
        switch (cmd[0]) {
            // Transmit a valid DALI command
            // case 'T':
            //     // We only accept 16 and 24 bit payloads for T
            //     if (!(len == 5 || len == 7)) {
            //         return CMD_BAD_INPUT;
            //     }
            //     uint32_t val = strtol(cmd+1, &endptr, 16);
            //     if (*endptr) {
            //         // We found an invalid character
            //         return CMD_BAD_INPUT;
            //     }
            //     transmit(val, (len-1)*4);
            //     return CMD_DEFERRED_RESPONSE;

            // Clear out any accumulated pulses. 
            // case 'C': 
            //     resetPulseArray();
            //     return CMD_OK;

            // append a pulse.
            // case 'P': 
            //     pw = USEC_TO_TICKS(atoi(cmd+1));
            //     // Minimum pulsewidth is 10 uSec
            //     if (pw < 34) {
            //         return CMD_BAD_INPUT;
            //     }
            //     if (isPulseArrayFull()) {
            //         return CMD_FULL;
            //     }

            //     *pulsePtr++ = pw;
            //     *pulsePtr = 0; // Ensure that pulse array always ends with a 0.
            //     log_info("Added pulse of %d ticks", pw);
            //     return CMD_OK;

            // Send our accumulated waveform. 
            // case 'W': 
            //     if (getPulseCount() % 2 == 0) {
            //         log_info("Error - expected odd number of pulse durations");
            //         return CMD_BAD_INPUT;
            //     }            
            //     transmit(sendCommandResponse);
            //     return CMD_DEFERRED_RESPONSE;
            case 'L':
                log_uint8("Line level:", (AC0.STATUS & AC_STATE_bm) ? 1 : 0);
                return CMD_OK;
            case '0':
                enqueueCommand(COMMAND_Off, 0, NULL, NULL);
                return CMD_DEFERRED_RESPONSE;
            case '1':
                enqueueCommand(COMMAND_RecallMaxLevel, 0, NULL, NULL);
                return CMD_DEFERRED_RESPONSE;
        }
    }
    return CMD_BAD_INPUT;
}

int main(void) {
    // button_event_t buttonEvents[NUM_BUTTONS];
    console_init(command_parser);

    retrieveConfig();

    // Set PB2 as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Use PA7 as an Analog Comparator, with reference of 0.55V
    // This makes it trigger sooner than if we were doing digital I/O
    VREF.CTRLA = VREF_DAC0REFSEL_0V55_gc;
    PORTA.PIN7CTRL  = PORT_ISC_INPUT_DISABLE_gc; // Disable Digital I/O
    AC0.MUXCTRLA = AC_MUXNEG_VREF_gc | AC_MUXPOS_PIN0_gc;
    AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm;
    EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_AC0_OUT_gc;
    EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc; // Set TCB0 to use ASYNCHCH0   

    initialise_timers();
    buttons_init();
    transmitNextCommandOrWaitForRead();
    sei();

    while (1) {
        _delay_ms(1);
    }
    return 0;
}