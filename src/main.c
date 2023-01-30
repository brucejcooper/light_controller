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
#include "cmd.h"
#include "timing.h"
#include "buttons.h"
#include "config.h"

uint8_t currentLevels[5];



static command_response_t command_processor(char *cmd) {
    uint8_t lvl;
    uint8_t op = 0xFF;


    // log_info(cmd);
    if (strlen(cmd) == 0) {
        return CMD_NO_OP;
    }
    if (strcmp(cmd, "ac") == 0) {
        log_uint8("AC status", AC0.STATUS);
        return CMD_OK;
    }

    if (strcmp(cmd, "off") == 0) {
        op = DALI_CMD_OFF;
    }
    if (strcmp(cmd, "on") == 0) {
        op = DALI_CMD_GO_TO_LAST_ACTIVE_LEVEL;
    }
    if (strcmp(cmd, "q") == 0) {
        op = DALI_CMD_QUERY_ACTUAL_LEVEL;
    }


    if (op != 0xFF) {
        read_result_t res = send_dali_cmd(config->targets[0], op, &lvl);
        switch (res) {
            case READ_NAK:
                log_info("No response");
                break;
            case READ_COLLISION:
                log_info("collision");
                break;
            case READ_VALUE:
                log_uint8("Response", lvl);
                break;

        }
        return CMD_OK;
    }
    return CMD_FAIL;
}


int main(void) {
    // button_event_t buttonEvents[NUM_BUTTONS];
    console_init(command_processor);
    log_uint8("Num Buttons", config->numButtons);
    for (uint8_t i = 0; i < config->numButtons; i++) {
        log_uint8("Target", config->targets[0]);
    }
    log_uint16("Short", config->shortPressTimer);
    log_uint16("Long", config->doublePressTimer);
    log_uint16("Repeat", config->repeatTimer);
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
    // EVSYS.ASYNCCH0 = EVSYS_ASYNCCH0_AC0_OUT_gc;  // Async Channel 0 is sourced from AC0
    // EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc; // ASYNCHCH0 destination is 0 (TCB0)

    AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm; // Enable the AC.


    // Make receiving the most important Interrupt - We want to deprioritise UART.
    CPUINT.LVL0PRI = TCB0_INT_vect_num;
    
    // Turn on the RTC
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV2_gc;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);


    buttons_init();
    sei();

    while (1) {
        bool all_idle = poll_buttons();
        // log_uint8("Level", AC0.STATUS);

        // if (all_idle) {
        //     log_info("Sleeping");
        //     sleep_mode();
        // }
    }
    return 0;
}