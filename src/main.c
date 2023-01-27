#define __DELAY_BACKWARD_COMPATIBLE__

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
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
#include "incoming_commands.h"
#include "config.h"
#include "timers.h"
#include "sched_callback.h"

uint8_t currentLevels[5];


static void updateButtonStatus(outgoing_command_t cmd, uint8_t index, outgoing_command_response_type_t responseType, uint8_t response, void *context) {
    switch (responseType) {
        case COMMAND_RESPONSE_ERROR:
            log_info("Error from status");
            break;
        case COMMAND_RESPONSE_NACK:
            log_info("NACK from status");
            break;
        case COMMAND_RESPONSE_VALUE:
            currentLevels[index-1] = response;
            log_uint8("index", index);
            log_uint8("lvl", response);
            break;
    };
}


void process_button_event(void *args) {
    button_event_t *evt = (button_event_t *) args;
    log_uint8("Button Event", evt->index << 4 | evt->type);

    switch (evt->type) {
        case EVENT_PRESSED:
            enqueueCommand(COMMAND_QueryActualLevel, evt->index, updateButtonStatus, NULL);
            break;
        case EVENT_RELEASED:
            if (currentLevels[evt->index-1]) {
                // Its currently on, so turn it off
                enqueueCommand(COMMAND_Off, evt->index, NULL, NULL);
            } else {
                // Its currently off, so turn it on to whatever its last active value was. 
                enqueueCommand(COMMAND_GoToLastActiveLevel, evt->index, NULL, NULL);
            }
            break;
        default:
            break;
    }
}

void command_observer(void *args) {
    command_event_t *cmd = (command_event_t *) args;

    log_info("Observe");

}

static command_response_t command_processor(char *cmd) {
    // log_info(cmd);
    if (strlen(cmd) == 0) {
        return CMD_NO_OP;
    }
    return CMD_FAIL;
}


int main(void) {
    // button_event_t buttonEvents[NUM_BUTTONS];
    console_init(command_processor);
    retrieveConfig();
    log_uint8("Short", config.shortAddr);
    log_uint8("T1", config.targets[0]);
    // log_uint8("T2", config.targets[1]);
    // log_uint8("T3", config.targets[2]);
    // log_uint8("T4", config.targets[3]);
    // log_uint8("T5", config.targets[4]);

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

    // Make receiving the most important Interrupt - We want to deprioritise UART.
    CPUINT.LVL0PRI = TCB0_INT_vect_num;

    initialise_timers();
    setCommandObserver(command_observer);
    buttons_init(process_button_event);
    transmitNextCommandOrWaitForRead();
    sei();

    while (1) {
            poll_calls();
    }
    return 0;
}