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
#include <avr/wdt.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "queue.h"
#include "buttons.h"



queue_t event_queue;


int mapButtonToAddress(int button) {
    // TODO implement me.
    return 0xde;
}

int mapActionToDaliCommand(button_event_t action) {
    // TODO implement me.
    return 0xad;
}

int main(void)
{
    uint8_t evt;
    queue_result_t res; 
    dali_result_t dres;
    int button;
    button_event_t action;
    uint8_t address;
    uint8_t command;

    // Write a WDT value, using the CCP method
    // CCP = CCP_IOREG_gc;
    // WDT.STATUS = WDT_LOCK_bm;
    // WDT.CTRLA = WDT_PERIOD_1KCLK_gc; // 1 second.

    dali_init();
    queue_init(&event_queue, 10);
    buttons_init(&event_queue);


    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sei();



    while (1) {
        // wdt_reset();

        // dres = dali_receive(&address, &command);
        // if (dres == DALI_OK) {
        //     // TODO deal with what we've read in as a command.
        // }



        res = queue_pop(&event_queue, &evt);
        if (res == QUEUE_OK) {
            button = evt & 0x0F;
            action = (button_event_t) evt >> 4;

            address = mapButtonToAddress(button);
            command = mapActionToDaliCommand(action);
            do {
                dres = dali_transmit_cmd(address, command);
                // TODO enforce required backoff if there was a collision - Random backoff time?
            } while (dres != DALI_OK);
            _delay_ms(5);
        } else {
            // TODO how to sleep with the WDT?  Disable it?
            // We wait for either an edge interrupt on Read, or an edge interrupt on one of the buttons.
            sleep_mode();
        }
    }
}
