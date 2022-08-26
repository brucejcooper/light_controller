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
#include "console.h"


// Switch is on PB2
#define SWITCH_PORT PORTB

// Debounce happens for 20 ms
#define DEBOUNCE_PERIOD 20
// Press becomes a long press after 1 sec (plus the debounce)
#define LONGPRESS_DELAY 600
// Long press repeat happens every 250 ms once longpress starts
#define LONGPRESS_REPEAT 250

#define DOUBLE_CLICK_PERIOD 200

#define MS_TO_TICKS(x) ((x)/10)

#define NUM_BUTTONS (1)


typedef struct button_state_t button_state_t;

typedef struct {
    const button_event_t action; 
    const button_state_t *next;
} state_transition_t;


typedef struct button_state_t {
    const uint16_t timeout;
    const state_transition_t onTimeoutPressed;
    const state_transition_t onTimeoutReleased;
    const state_transition_t onToggle;
} button_state_t;


typedef struct button_t {
    const button_state_t *state;
    uint16_t ticks_remaining;
    uint8_t mask;
    uint16_t last_release;
    uint8_t val;
} button_t;
 


#define NO_TRANSITION { .action = EVENT_NONE, .next = NULL }


static const button_state_t idleState;
static const button_state_t debounceState;
static const button_state_t firstPressState;
static const button_state_t firstClickReleasedState;
static const button_state_t longPressedState;
static const button_state_t debounceDoubleClickState;
static const button_state_t doubleClickPressedState;
static const button_state_t debounceReleasedState;
static const button_state_t firstPressReleaseDebounceState;

static const button_state_t idleState = {
    .timeout = 0,
    .onTimeoutPressed = NO_TRANSITION,
    .onTimeoutReleased = NO_TRANSITION,
    .onToggle = { .next = &debounceState },
};

static const button_state_t debounceState = {
    .timeout = MS_TO_TICKS(DEBOUNCE_PERIOD),
    .onTimeoutPressed = { .next = &firstPressState },
    .onTimeoutReleased = { .next = &firstClickReleasedState },
    .onToggle = NO_TRANSITION,
};

static const button_state_t firstPressState = {
    .timeout = MS_TO_TICKS(LONGPRESS_DELAY),
    .onTimeoutPressed = { .action = EVENT_DIMMER_BRIGHTEN, .next = &longPressedState },
    .onTimeoutReleased = { .next = &firstPressReleaseDebounceState },
    .onToggle = { .next = &firstPressReleaseDebounceState }, 
};


static const button_state_t firstPressReleaseDebounceState = {
    .timeout = MS_TO_TICKS(DEBOUNCE_PERIOD),
    .onTimeoutPressed = { .next = &doubleClickPressedState },
    .onTimeoutReleased = { .next = &firstClickReleasedState },
    .onToggle = NO_TRANSITION, 
};


// It is released, but we're not sure if its a double click or not yet.  Wait for up to DOUBLE_CLICK_PERIOD for another press.
static const button_state_t firstClickReleasedState = {
    .timeout = MS_TO_TICKS(DOUBLE_CLICK_PERIOD),
    .onTimeoutPressed = { .next = &firstPressState }, // It would be odd to end up here.
    .onTimeoutReleased = { .action = EVENT_TOGGLE, .next = &idleState }, // It timed out, so its a single click then release (TODO issue output)
    .onToggle =  {.next = &debounceDoubleClickState }, 
};

// We've double clicked, but we're waiting for the line to settle.
static const button_state_t debounceDoubleClickState = {
    .timeout = MS_TO_TICKS(DEBOUNCE_PERIOD),
    .onTimeoutPressed = { .action = EVENT_DOUBLE_CLICK, .next = &doubleClickPressedState },   // Still pressed after double click. 
    .onTimeoutReleased = { .action = EVENT_DOUBLE_CLICK, .next = &debounceReleasedState }, // Was released during debounce, so its a normal double click -> idle.
    .onToggle = NO_TRANSITION, 
};


static const button_state_t doubleClickPressedState = {
    .timeout = MS_TO_TICKS(LONGPRESS_REPEAT),
    .onTimeoutPressed = { .action =  EVENT_DIMMER_DIM }, 
    .onTimeoutReleased = { .next = &debounceReleasedState },
    .onToggle = { .next = &debounceReleasedState }, 
};

// We transition here when the button has been released and there are no more double clicks to worry about.
static const button_state_t debounceReleasedState = {
    .timeout = MS_TO_TICKS(DEBOUNCE_PERIOD),
    .onTimeoutPressed = { .next = &firstPressState },
    .onTimeoutReleased = { .next = &idleState },
    .onToggle = NO_TRANSITION, 
};

static const button_state_t longPressedState = {
    .timeout = MS_TO_TICKS(LONGPRESS_REPEAT),
    .onTimeoutPressed = { .action = EVENT_DIMMER_BRIGHTEN },
    .onTimeoutReleased = { .next = &debounceReleasedState },
    .onToggle = { .next = &debounceReleasedState }, 
};



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


void executeTransition(int pin, volatile button_t *btn, const state_transition_t *transition) {
    if (transition->action != EVENT_NONE) {
        queue_push(eventQueue, makeEvent(pin, transition->action));
    }
    if (transition->next) {
        btn->state = transition->next;
    }
    btn->ticks_remaining = btn->state->timeout;
}

// Set up a timer to go off every 10 milliseconds
// Timer runs while one or more button is pressed.
// Each time timer goes off, decrement next_button_timeout (unless not enabled for that button) - When it reaches zero, the action is triggered (potentially restarting the timer)

// RTC Timer - Goes off every 10ms (100/sec)
ISR(RTC_CNT_vect)
{
    if (RTC.INTFLAGS & RTC_OVF_bm) {

        for (int i = 0; i < NUM_BUTTONS; i++) {
            volatile button_t *btn = &buttons[i];

            if (btn->ticks_remaining > 0 && --(btn->ticks_remaining) == 0) {
                executeTransition(i, btn, ((SWITCH_PORT.IN & btn->mask) == 0) 
                                            ? &(btn->state->onTimeoutPressed) 
                                            : &(btn->state->onTimeoutReleased));
            }
        }
        RTC.INTFLAGS |= RTC_OVF_bm; // Clear flag
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
            SWITCH_PORT.INTFLAGS = btn->mask;          

            executeTransition(pin, btn, &(btn->state->onToggle));
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
    // buttons[1].mask = PIN3_bm;
    // buttons[2].mask = PIN4_bm;
    // buttons[3].mask = PIN5_bm;
    // buttons[4].mask = PIN6_bm;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].state = &idleState;
        buttons[i].ticks_remaining = 0;
        buttons[i].last_release = 0;
        mask |= buttons[i].mask;
    }
    initialise_rtc();


    SWITCH_PORT.DIRCLR = mask; // Set all buttons as inputs.
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].val = SWITCH_PORT.IN & buttons[i].mask ? 1 : 0;

    }
    // Enable PullUp.
    SWITCH_PORT.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}


