#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "buttons.h"
#include "console.h"
#include "timers.h"
#include "outgoing_commands.h"
#include "state_machine.h"


// Switch is on PB2
#define SWITCH_PORT PORTA

#define DEBOUNCE_TICKS 20
#define LONGPRESS_START_TICKS 512
#define LONGPRESS_REPEAT_TICKS 256



struct button_t;

typedef button_event_t (*state_handler_t)(struct button_t *btn, uint8_t newVal);

typedef struct button_t {
    uint8_t mask;
    uint8_t click_count;
    timer_t timer;
} button_t;

/**
 * @brief Timeout for when next action will happen 
 * if positive, this is the number of ticks until the next action for that button
 * if negative, then timer is disabled for that button.
 */
button_t buttons[NUM_BUTTONS];

static void button_debounced(void *ctx);
static void button_released_debounce(void *ctx);

static void button_pressed(button_t *btn) {
    log_info("Pressed");
    // enqueueCommand(COMMAND_QueryActualLevel, 0, NULL, NULL);

    // TODO emit a button press event
    // Wait for the debounce period before we do anything else (we'll check level then)
    startTimer(DEBOUNCE_TICKS, button_debounced, btn, &(btn->timer));
}

static void button_released(button_t *btn) {
    log_info("Released");
    // Its a release
    // cancel any existing timers for this button.
    cancelTimer(&btn->timer);
    // TODO emit a button release event

    // Wait for the debounce period before we do anything else (we'll check level then)
    startTimer(DEBOUNCE_TICKS, button_released_debounce, btn, &(btn->timer));    
}


ISR(PORTA_PORT_vect)
{
    for (int pin = 0; pin < NUM_BUTTONS; pin++) {
        button_t *btn = &buttons[pin];

        if(SWITCH_PORT.INTFLAGS & btn->mask) {
            uint8_t evt = SWITCH_PORT.PIN6CTRL & PORT_ISC_gm;
            // Disable futher interrupts (for now);
            SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm;
            switch (evt) {
                case PORT_ISC_LEVEL_gc: 
                    button_pressed(btn);
                    break;
                case PORT_ISC_RISING_gc:
                    button_released(btn);
                    break;
                default:
                    log_uint8("Illegal Port interrupt %d", SWITCH_PORT.PIN6CTRL & PORT_ISC_gm);
                    break;
            }
            // Clear the interrupt flag.
            SWITCH_PORT.INTFLAGS = btn->mask;          
        }
    }
}



static void wait_for_press(button_t *btn) {
    // Enable interrupt on low value
    SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;
    // clear any existing interrupt
    SWITCH_PORT.INTFLAGS = btn->mask;
}

static void wait_for_release(button_t *btn) {
    // Enable interrupt on low value
    SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
    // clear any existing interrupt
    SWITCH_PORT.INTFLAGS = btn->mask;
}

void buttons_init() {
    // combine all the button masks into one mask.
    uint8_t mask = 0;
    buttons[0].mask = PIN6_bm;
    // buttons[1].mask = PIN3_bm;
    // buttons[2].mask = PIN4_bm;
    // buttons[3].mask = PIN5_bm;
    // buttons[4].mask = PIN6_bm;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        mask |= buttons[i].mask;
    }

    SWITCH_PORT.DIRCLR = mask; // Set all buttons as inputs.
    // Read initial values.
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].click_count = 0;

        if (SWITCH_PORT.IN & buttons[i].mask) {
            wait_for_press(&buttons[i]);
        } else {
            wait_for_release(&buttons[i]);
        }
    }
}


static void long_press_repeated(void *ctx) {
    // TODO emit event
    log_info("lp repeat");

    button_t *btn = (button_t *) ctx;
    startTimer(LONGPRESS_REPEAT_TICKS, long_press_repeated, btn, &(btn->timer)); // start repeating after half a second.    
}


static void button_long_pressed(void *ctx) {
    log_info("long_press");
    // TODO emit event
    button_t *btn = (button_t *) ctx;
    startTimer(LONGPRESS_REPEAT_TICKS, long_press_repeated, btn, &(btn->timer)); // start repeating after half a second.
}


static void button_debounced(void *ctx) {
    log_info("Db");

    button_t *btn = (button_t *) ctx;

    if(SWITCH_PORT.IN & btn->mask) {
        // Released during debounce.
        // TODO Emit button released event.
        button_released(btn);
    } else {
        wait_for_release(btn);
        // Change Pin ISR to respond to releases
        // Still pressed, start waiting for long presses or release.
        startTimer(LONGPRESS_START_TICKS, button_long_pressed, btn, &(btn->timer)); // start repeating after half a second.
    }
}


static void button_released_debounce(void *ctx) {
    log_info("Dbr");
    button_t *btn = (button_t *) ctx;

    if(SWITCH_PORT.IN & btn->mask) {
        // It remains released.
        wait_for_press(btn);
    } else {
        // It was re-pressed and remains so - treat it as a new press.
        button_pressed(btn);
    }
}
