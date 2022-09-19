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
#define SWITCH_PORT PORTA

#define DEBOUNCE_TICKS 20
#define LONGPRESS_START_TICKS 512
#define LONGPRESS_REPEAT_TICKS 256



struct button_t;

typedef button_event_t (*state_handler_t)(struct button_t *btn, uint8_t newVal);

typedef struct button_t {
    state_handler_t state;
    uint16_t action_timeout;
    uint8_t mask;
    uint16_t last_release;
    uint8_t click_count;
    uint8_t val;
} button_t;

/**
 * @brief Timeout for when next action will happen 
 * if positive, this is the number of ticks until the next action for that button
 * if negative, then timer is disabled for that button.
 */
button_t buttons[NUM_BUTTONS];

button_event_t button_idle(button_t *btn, uint8_t newVal);
button_event_t button_debounce(button_t *btn, uint8_t newVal);
button_event_t button_pressed(button_t *btn, uint8_t newVal);
button_event_t button_released_debounce(button_t *btn, uint8_t newVal);


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


ISR(PORTA_PORT_vect)
{
    for (int pin = 0; pin < NUM_BUTTONS; pin++) {
        volatile button_t *btn = &buttons[pin];

        if(SWITCH_PORT.INTFLAGS & btn->mask) {
            // Clear the interrupt flag.
            SWITCH_PORT.INTFLAGS = btn->mask;          
        }
    }
}




static void initialise_rtc() {
    // Configure RTC clock.
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // Use 1.024 Khz clock
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_PRESCALER_DIV1_gc;
    enable_rtc();
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
    initialise_rtc();

    SWITCH_PORT.DIRCLR = mask; // Set all buttons as inputs.
    // Read initial values.
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].val = SWITCH_PORT.IN & buttons[i].mask ? 1 : 0;
        buttons[i].last_release = 0;
        buttons[i].click_count = 0;

        if (buttons[i].val) {
            buttons[i].state = button_idle;
        } else {
            buttons[i].state = button_debounce;
        }
    }
    // Enable PullUp.
    SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm;
}


static inline bool is_timed_out(button_t *btn) {
    return (int16_t) (RTC.CNT - btn->action_timeout) > 0;
}


button_event_t button_released_debounce(button_t *btn, uint8_t newVal) {
    if (is_timed_out(btn)) {
        if (newVal == 0) {
            // It was pressed again during debounce.
            btn->state = button_debounce;
            btn->action_timeout = RTC.CNT + DEBOUNCE_TICKS; // Wait for 15ish milliseconds
            return EVENT_PRESSED;
        } else {
            // It was released
            btn->state = button_idle;
        }

    }
    return EVENT_NONE;
}


button_event_t button_pressed(button_t *btn, uint8_t newVal) {
    if (newVal) {
        // It was released
        btn->state = button_released_debounce;
        btn->action_timeout = RTC.CNT + DEBOUNCE_TICKS; // Wait for 15ish milliseconds
        btn->last_release = RTC.CNT;
        return EVENT_RELEASED;
    }  else {
        if (is_timed_out(btn)) {
            // Change timeout to be a bit faster for subsequent actions
            btn->action_timeout = RTC.CNT + LONGPRESS_REPEAT_TICKS; // 1/4 second
            return EVENT_LONG_PRESSED;
        }
    }
    return EVENT_NONE;

}



button_event_t button_debounce(button_t *btn, uint8_t newVal) {
    if (is_timed_out(btn)) {
        if (newVal == 0) {
            // Still pressed, start waiting for long presses or releases.
            btn->state = button_pressed;
            btn->action_timeout = RTC.CNT + LONGPRESS_START_TICKS; // start repeating after half a second.
        } else {
            // Released during debounce.
            btn->state = button_idle;
            btn->last_release = RTC.CNT;
            return EVENT_RELEASED;
        }
    }
    return EVENT_NONE;
}



button_event_t button_idle(button_t *btn, uint8_t newVal) {
    if (newVal == 0) {
        btn->state = button_debounce;
        btn->action_timeout = RTC.CNT + DEBOUNCE_TICKS; // Wait for 15ish milliseconds
        return EVENT_PRESSED;
    }
    return EVENT_NONE;
}




bool scan_buttons(button_event_t event[NUM_BUTTONS]) {
    bool allIdle = true;
    uint8_t portVal = SWITCH_PORT.IN;
    button_t *btn = buttons;

    for (int i = 0; i < NUM_BUTTONS; i++, btn++) {
        uint8_t newVal = portVal & btn->mask ? 1 : 0;
        event[i] = btn->state(btn, newVal);
        btn->val = newVal;
        
        if (btn->state != button_idle) {
            allIdle = false;
        }
    }
    return allIdle;
}


void buttons_set_wake_from_sleep_enabled(bool enabled) {
    SWITCH_PORT.PIN6CTRL = enabled ? PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc : PORT_PULLUPEN_bm;

}