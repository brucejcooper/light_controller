/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "queue.h"
#include "buttons.h"


// Switch is on PB2
#define SWITCH_PORT PORTB

// Debounce happens for 20 ms
#define DEBOUNCE_PERIOD 20
// Press becomes a long press after 1 sec (plus the debounce)
#define LONGPRESS_DELAY 1000
// Long press repeat happens every 250 ms once longpress starts
#define LONGPRESS_REPEAT 250

#define MS_TO_TICKS(x) ((x)/10)


/*
 * Events that can occur 
 * 1. a button short press (x 5 buttons) - GPIO interrupt, plus timers for debounce (too short a press should be ignored as a debounce)
 * 2. a button long press (x 5 buttons) - as above, but the timer is ongoing. 
 * 3. There is a DALI command to send (in the queue) and the bus is idle.
 * 4. a DALI receive starts - GPIO change interrupt plus timing (on the order of milliseconds).
 * 
 */

#define NUM_BUTTONS (5)

typedef enum {
    BTN_RELEASED,
    BTN_PRESSED_DEBOUNCE,
    BTN_PRESSED,
    BTN_LONG_PRESSED,
    BTN_RELEASED_DEBOUNCE,
} switchstate_t;

typedef struct {
    switchstate_t state;
    uint16_t ticks_remaining;
    uint8_t mask;
} button_t;
 
/**
 * @brief Timeout for when next action will happen 
 * if positive, this is the number of ticks until the next action for that button
 * if negative, then timer is disabled for that button.
 */
volatile button_t buttons[NUM_BUTTONS];
queue_t *eventQueue;

/**
 * @brief maxes a button and event into a uint8_t for the queue.  Event is high nibble, button index is low.
 * 
 * @return uint8_t The combined value
 */
uint8_t makeEvent(uint8_t button, button_event_t evt) {
    return (button & 0x0f) | evt << 4;
}


// Set up a timer to go off every 1 millisecond
// Timer runs while one or more button is pressed.
// Each time timer goes off, decrement next_button_timeout (unless not enabled for that button) - When it reaches zero, the action is triggered (potentially restarting the timer)

// RTC Timer - Goes off every 10ms (100/sec)
ISR(RTC_CNT_vect)
{
    if (RTC.INTFLAGS & RTC_OVF_bm) {
        RTC.INTFLAGS |= RTC_OVF_bm; // Clear flag

        for (int i = 0; i < NUM_BUTTONS; i++) {
            volatile button_t *btn = &buttons[i];

            if (btn->ticks_remaining > 0) {
                if (--(btn->ticks_remaining) == 0) {
                    switch (btn->state) {
                        case BTN_PRESSED_DEBOUNCE:
                            if ((SWITCH_PORT.IN & btn->mask) == 0) {
                                // Still Pressed
                                btn->state = BTN_PRESSED;
                                btn->ticks_remaining = MS_TO_TICKS(LONGPRESS_DELAY);
                            } else {
                                // released during debounce period
                                btn->state = BTN_RELEASED;
                                queue_push(eventQueue, makeEvent(i, EVENT_TOGGLE));

                            }
                            break;
                        case BTN_RELEASED_DEBOUNCE:
                            if (SWITCH_PORT.IN & btn->mask) {
                                // Button still released
                                btn->state = BTN_RELEASED;
                            } else {
                                // Button re-pressed during debounce.... 
                                btn->state = BTN_PRESSED;
                                btn->ticks_remaining = MS_TO_TICKS(LONGPRESS_DELAY);
                            }
                            break;
                        case BTN_RELEASED:
                            // Shouldn't happen.  Do nothing, and the timer will not re-start.
                            break;
                        case BTN_PRESSED:
                        case BTN_LONG_PRESSED:
                            // TODO make it go both ways.
                            queue_push(eventQueue, makeEvent(i, EVENT_DIMMER_DIM));
                            btn->state = BTN_LONG_PRESSED;
                            btn->ticks_remaining = MS_TO_TICKS(LONGPRESS_REPEAT);
                            break;
                    }
                }
            }
        }
    }
}


static inline void disable_rtc() {
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA &= ~RTC_RTCEN_bm;
}

static inline void enable_rtc() {
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA |= RTC_RTCEN_bm;
}


ISR(PORTB_PORT_vect)
{
    for (int pin = 0; pin < NUM_BUTTONS; pin++) {
        volatile button_t *btn = &buttons[pin];

        if(SWITCH_PORT.INTFLAGS & btn->mask) {
            // Clear the interrupt flag.
            SWITCH_PORT.INTFLAGS &= btn->mask;          

            switch (btn->state) {
                case BTN_PRESSED_DEBOUNCE:
                case BTN_RELEASED_DEBOUNCE:
                    // Do nothing during debounce period
                    break;
                case BTN_RELEASED:
                    if ((SWITCH_PORT.IN & btn->mask) == 0) {
                        // Button has been pressed
                        btn->state = BTN_PRESSED_DEBOUNCE;
                        btn->ticks_remaining = MS_TO_TICKS(DEBOUNCE_PERIOD);
                    }
                    break;
                case BTN_PRESSED:
                    if (SWITCH_PORT.IN & btn->mask) {
                        // Button has been released from its pressed state before long press kicked in
                        queue_push(eventQueue, makeEvent(pin, EVENT_TOGGLE));
                        btn->state = BTN_RELEASED_DEBOUNCE;
                        btn->ticks_remaining = MS_TO_TICKS(DEBOUNCE_PERIOD);
                    }
                    break;
                case BTN_LONG_PRESSED:
                    if (SWITCH_PORT.IN & btn->mask) {
                        // Button has been released
                        btn->state = BTN_RELEASED_DEBOUNCE;
                        btn->ticks_remaining = MS_TO_TICKS(DEBOUNCE_PERIOD);
                    }
                    break;
            }
        }
    }
}




static void initialise_rtc() {
    // Configure RTC clock.
    RTC.CLKSEL = RTC_CLKSEL_INT32K_gc; // Fetch clock from 32.768khz RTC timer.
    while (RTC.STATUS & RTC_PERBUSY_bm) {
        ;
    }
    RTC.PER = 328; // PIT will go off every 10ms.
    RTC.INTCTRL = RTC_OVF_bm; // Interrupt on overflow only.
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc | RTC_RUNSTDBY_bm; // RTC runs in standby.
    enable_rtc();

}


void buttons_init(queue_t *q) {
    eventQueue = q;
    // combine all the button masks into one mask.
    uint8_t mask = 0;
    buttons[0].mask = PIN1_bm;
    buttons[1].mask = PIN2_bm;
    buttons[2].mask = PIN3_bm;
    buttons[3].mask = PIN4_bm;
    buttons[4].mask = PIN5_bm;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].state = BTN_RELEASED;
        buttons[i].ticks_remaining = 0;
        mask |= buttons[i].mask;
    }
    SWITCH_PORT.DIRCLR = mask; // Set all buttons as inputs.
    SWITCH_PORT.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc; /* enable internal pull-up, and interrupts on both edges */

    initialise_rtc();
}


