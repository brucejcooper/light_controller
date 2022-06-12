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
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "queue.h"
#include "buttons.h"



queue_t event_queue;


int mapButtonToAddress(int button) {
    // TODO implement me.
    return button+1;
}

int mapActionToDaliCommand(button_event_t action) {
    // TODO implement me.
    return 0x01;
}


static void debug() {
    if (PORTA.OUT & PIN4_bm) {
        PORTA.OUT &= ~PIN4_bm;
    } else {
        PORTA.OUT |= PIN4_bm;
    }

}


int main(void)
{
    dali_init();
    queue_init(&event_queue, 10);
    buttons_init(&event_queue);


    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sei();

    while (1)
    {
        uint8_t evt;
        queue_result_t res = queue_pop(&event_queue, &evt);
        dali_result_t dres;
        int button;
        button_event_t action;
        uint8_t address;
        uint8_t command;

        if (res == QUEUE_OK) {
            button = evt & 0x0F;
            action = (button_event_t) evt >> 4;

            address = mapButtonToAddress(button);
            command = mapActionToDaliCommand(action);

            debug();

            // do {
                // dres = dali_transmit_cmd(address, command);
                // TODO enforce required backoff if there was a collision - Random backoff time?
            // } while (dres != DALI_OK);
        } else {
            // sleep_mode();
        }
    }
}
