#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <stdbool.h>
#include <string.h>
#include "buttons.h"
#include "console.h"
#include "config.h"
#include <stdlib.h>
#include "cmd.h"

// For test board, switch is on PA6
#define SWITCH_PORT PORTA
#define MS_TO_RTC_TICKS(m) (m * 32768 / 1000 / 2)
struct button_t;

typedef enum {
    BTN_STATE_RELEASED,
    BTN_STATE_PRESSED,
    BTN_STATE_RELEASED_DEBOUNCING,
    BTN_STATE_LONGHELD,
} button_state_t; 


typedef void (*state_handler_t)(struct button_t *btn, const uint8_t button_level);
typedef struct button_t {
    // Index of the button.
    uint8_t index;
    // Counts how many times the button has been clicked to switch fade direction
    uint8_t click_count;
    // RTC timeout for next 
    uint16_t timeout;
    // Bit mask for the pin
    uint8_t mask;
    // The state handler.
    button_state_t state;
    // The last read light level for this target.
    uint8_t light_level;
} button_t;

static void released(button_t *btn, const uint8_t button_level);
static void pressed(button_t *btn, const uint8_t button_level);
static void long_pressed(button_t *btn, const uint8_t button_level);
static void released_debouncing(button_t *btn, const uint8_t button_level);
static void long_pressed(button_t *btn, const uint8_t button_level);

static const button_t buttons[NUM_BUTTONS] = {
    {
        .state = BTN_STATE_RELEASED,
        .mask = PIN6_bm,
        .light_level = 0,
        .click_count = 0,
        .timeout = 0,
    }
};

void buttons_init() {
    // Set up pins with pullup, and interrupt on 
    SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;
    // Set all the pins as inputs.
    SWITCH_PORT.DIRCLR = PIN6_bm;
}

static inline bool is_timer_expired(button_t *btn) {
    return ((int16_t) (RTC.CNT - btn->timeout)) >= 0;
}


static void released(button_t *btn, const uint8_t button_level) {
    if (!button_level) {
        // Its been pressed.
        btn->state = BTN_STATE_PRESSED;
        btn->timeout = RTC.CNT + config->doublePressTimer;

        // Whilst we're waiting for the button to debounce, ask the ballast its current level.
        // Because this takes some time (a little over 20 ms, post response delay), there is no 
        // need for an additional debounce timer.
        read_result_t res = send_dali_cmd(config->targets[btn->index], DALI_CMD_QUERY_ACTUAL_LEVEL, &btn->light_level);            
        if (res != READ_VALUE) {
            log_info("Did not receive response from ballast");
            btn->light_level = 0; // default to off, meaning when we release the light should be turned on.
        }
    }
}

static void pressed(button_t *btn, const uint8_t button_level) {
    uint8_t dummy;
    if (button_level) {
        // Its been released - send out either an off or an on command, depending ont he current level
        log_uint8("Released", btn->index);
        log_uint8("Old value ", btn->light_level);
        read_result_t res = send_dali_cmd(config->targets[btn->index], btn->light_level ? DALI_CMD_OFF : DALI_CMD_GO_TO_LAST_ACTIVE_LEVEL, &dummy);
        if (res != READ_NAK) {
            log_info("Received unexpected response from command");
        }
        // The command will have taken a little over 10 milliseconds.  This is effectively our debounce period.
        btn->state = BTN_STATE_RELEASED;
    } else if (is_timer_expired(btn)) {
        log_info("Long pressed");
        btn->state = BTN_STATE_LONGHELD;
        btn->timeout = RTC.CNT + config->repeatTimer; 
    }
}

static void long_pressed(button_t *btn, const uint8_t button_level) {
    if (button_level) {
        // It was released. Do some debouncing.
        btn->state = BTN_STATE_RELEASED_DEBOUNCING;
        btn->timeout = RTC.CNT + MS_TO_RTC_TICKS(10);
    } else if (is_timer_expired(btn)) {
        // Its been held long enough now for a repeat.
        btn->timeout = RTC.CNT + config->repeatTimer; 
        log_uint8("Repeat", btn->index);
    }
}

static void released_debouncing(button_t *btn, const uint8_t button_level) {
    if (is_timer_expired(btn)) {
        btn->state = BTN_STATE_RELEASED;
    }
}


bool poll_buttons() {
    bool all_idle = true;

    for  (button_t *btn = (button_t *) buttons; btn < (buttons+NUM_BUTTONS); btn++) {
        uint8_t val = SWITCH_PORT.IN & btn->mask;

        switch (btn->state) {
            case BTN_STATE_RELEASED:
            released(btn, val);
            break;
            case BTN_STATE_PRESSED:
            pressed(btn, val);
            break;
            case BTN_STATE_LONGHELD:
            long_pressed(btn, val);
            break;
            case BTN_STATE_RELEASED_DEBOUNCING:
            released_debouncing(btn, val);
            break;
        }
        if (btn->state != BTN_STATE_RELEASED) {
            all_idle = false;
        }
    }
    return all_idle;
}
