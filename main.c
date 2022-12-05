/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "queue.h"
#include "buttons.h"
#include "console.h"
#include <string.h>

typedef struct {
    uint8_t shortAddr;  
    uint8_t addrL; // Programmed in by the external master.... 
    uint8_t addrM;
    uint8_t addrH;
    uint8_t numButtons;
    uint8_t targets[5]; // The targets
} userdata_t;



const userdata_t *userData = (userdata_t *) (&USERROW);



int mapButtonToAddress(int button) {
    // TODO implement me.
    return 0xde;
}

int mapActionToDaliCommand(button_event_t action) {
    // TODO implement me.
    return 0xae;
}

void cmdTransmitted(bool collision) {
    dali_idle_state_enter();
    sendCommandResponse(collision? CMD_FAIL : CMD_OK);
}

static command_response_t processCommand(char *cmd) {
    dali_result_t dres;

    if (strlen(cmd) == 0) {
        return CMD_NO_OP;
    }
    if (strcmp(cmd, "reset") == 0) {

        return CMD_OK;
    }
    if (cmd[0] == '0') {
        dres = dali_queue_transmit(0xFF00, 16, cmdTransmitted);
        if (dres != DALI_OK) {
            log_info("error transmitting: %d", dres);
        }
        return CMD_DEFERRED_RESPONSE;
    }

    if (cmd[0] == '1') {
        dres = dali_queue_transmit(0xFF05, 16, cmdTransmitted);
        if (dres != DALI_OK) {
            log_info("error transmitting: %d", dres);
        }
        return CMD_OK;
    }
    // We don't support any commands.
    return CMD_DEFERRED_RESPONSE;
}

int main(void) {
    dali_result_t dres;
    button_event_t events[NUM_BUTTONS];
    bool allButtonsIdle;

    // Write a WDT value, using the CCP method
    // CCP = CCP_IOREG_gc;
    // WDT.STATUS = WDT_LOCK_bm;
    // WDT.CTRLA = WDT_PERIOD_1KCLK_gc; // 1 second.
    console_init(processCommand);


    log_info("HALF_BIT_MIN = %d", HALFTICK_MIN_TICKS);

    dali_init();
    buttons_init();

    set_sleep_mode(SLEEP_MODE_STANDBY);
    sei();


    while (1) {

        // wdt_reset();
        allButtonsIdle = scan_buttons(events);
        for (int i = 0; i < NUM_BUTTONS; i++) {
            switch (events[i]) {
                case EVENT_NONE:
                break;
                case EVENT_PRESSED:
                log_info("Button %d pressed", i);
                dres = dali_queue_transmit(0xFF00, 16, cmdTransmitted);

                if (dres != DALI_OK) {
                    log_info("error transmitting: %d", dres);
                }
                break;
                case EVENT_LONG_PRESSED:
                log_info("Button %d long pressed", i);
                break;
                case EVENT_RELEASED:
                log_info("Button %d released", i);
                break;
            }
        }

        // if (allButtonsIdle && !dali_transmitting) {
        //     buttons_set_wake_from_sleep_enabled(true);
        //     sleep_enable();
        //     sleep_cpu();
        //     sleep_disable();
        //     buttons_set_wake_from_sleep_enabled(false);
        // }

        // dres = dali_receive(&address, &command);
        // if (dres == DALI_OK) {
        //     // TODO deal with what we've read in as a command.
        // }
        
        // res = queue_pop(&event_queue, &evt);
        // if (res == QUEUE_OK) {
        //     button = evt & 0x0F;
        //     action = (button_event_t) evt >> 4;
        //     switch (action) {
        //         case EVENT_PRESSED:
        //             log_info("toggle %d", button);
        //             PORTA.OUTTGL = PIN7_bm;
        //             break;
        //         case EVENT_LONG_PRESSED:
        //             log_info("brighten %d", button);
        //             PORTA.OUTTGL = PIN7_bm;
        //             break;
        //         case EVENT_DIMMER_DIM:
        //             log_info("dim %d", button);
        //             PORTA.OUTTGL = PIN7_bm;
        //             break;
        //         default:
        //             // Do nothing
        //             break;
        //     }

        //     address = mapButtonToAddress(button);
        //     command = mapActionToDaliCommand(action);
        //     do {
        //         log_info("Transmitting 0x%02x%02x", address, command);
        //         dres = dali_transmit_cmd(address, command);
        //         dali_wait_for_transmission();
        //         log_info("Done transmitting");
        //         _delay_ms(5);
        //         // TODO enforce required backoff if there was a collision - Random backoff time?
        //     } while (dres != DALI_OK);
        // } else {
        //     // TODO how to sleep with the WDT?  Disable it?
        //     // We wait for either an edge interrupt on Read, or an edge interrupt on one of the buttons.
        //     sleep_cpu();
        // }
    }
    return 0;
}
