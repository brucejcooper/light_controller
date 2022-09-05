/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

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
#include "dali.h"
#include "queue.h"
#include "buttons.h"
#include "console.h"



queue_t event_queue;


int mapButtonToAddress(int button) {
    // TODO implement me.
    return 0xde;
}

int mapActionToDaliCommand(button_event_t action) {
    // TODO implement me.
    return 0xae;
}

int main(void)
{
    // uint8_t evt;
    // queue_result_t res; 
    dali_result_t dres;
    // int button;
    // button_event_t action;
    // uint8_t address;
    // uint8_t command;
    button_event_t events[NUM_BUTTONS];
    bool allButtonsIdle;

    // Write a WDT value, using the CCP method
    // CCP = CCP_IOREG_gc;
    // WDT.STATUS = WDT_LOCK_bm;
    // WDT.CTRLA = WDT_PERIOD_1KCLK_gc; // 1 second.

    console_init();

    // Start with the LED turned off
    PORTA.OUTSET = PIN7_bm;
    PORTA.DIRSET = PIN7_bm;




    dali_init();
    queue_init(&event_queue, 10);
    buttons_init(&event_queue);


    set_sleep_mode(SLEEP_MODE_STANDBY);
    sei();

    log_info("Started");


    while (1) {
        // wdt_reset();
        allButtonsIdle = scan_buttons(events);
        for (int i = 0; i < NUM_BUTTONS; i++) {
            switch (events[i]) {
                case EVENT_NONE:
                break;
                case EVENT_PRESSED:
                log_info("Button %d pressed", i);
                dres = dali_transmit_cmd(0xFE, 0xED);
                if (dres != DALI_OK) {
                    log_info("error transmitting: %d", dres);
                }
                dali_wait_for_transmission();
                break;
                case EVENT_LONG_PRESSED:
                log_info("Button %d long pressed", i);
                break;
                case EVENT_RELEASED:
                log_info("Button %d released", i);
                break;
            }
        }

        if (allButtonsIdle && !dali_transmitting) {
            buttons_set_wake_from_sleep_enabled(true);
            sleep_enable();
            sleep_cpu();
            sleep_disable();
            buttons_set_wake_from_sleep_enabled(false);
        }

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
}
