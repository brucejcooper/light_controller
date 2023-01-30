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

// Switch is on PB2
#define SWITCH_PORT PORTA
struct button_t;

typedef enum {
    BTN_STATE_RELEASED,
    BTN_STATE_DEBOUNCING,
    BTN_STATE_PRESSED,
    BTN_STATE_RELEASED_DEBOUNCING,
    BTN_STATE_LONGHELD,
} button_state_t; 

typedef struct button_t {
    uint8_t click_count;
    uint16_t next_action;
    uint8_t mask;
    button_state_t state;

    cmd_queue_entry_t command;
} button_t;


/**
 * @brief Timeout for when next action will happen 
 * if positive, this is the number of ticks until the next action for that button
 * if negative, then timer is disabled for that button.
 */
static button_t buttons[NUM_BUTTONS] = {
    {
        .state = BTN_STATE_RELEASED,
        .mask = PIN6_bm,
    }
};

void buttons_init() {
    SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        SWITCH_PORT.DIRCLR = buttons[i].mask; // Set all buttons as inputs.
        buttons[i].state = BTN_STATE_RELEASED;  // Start in released state -It will immediately go into button press if it is already held down.
    }
}

static inline bool is_timer_expired(button_t *btn) {
    return ((int16_t) (RTC.CNT - (btn->next_action))) > 0;
}



static void poll_button(uint8_t index, button_t *btn) {
    uint8_t val = SWITCH_PORT.IN & btn->mask;
    switch (btn->state) {
        case BTN_STATE_RELEASED:
        if (val == 0) {
            // Its been pressed.
            btn->state = BTN_STATE_DEBOUNCING;
            btn->next_action = RTC.CNT + config->shortPressTimer;

            // query what the current level is.
            btn->command.cmd = config->targets[index] << 8 | DALI_CMD_QUERY_ACTUAL_LEVEL;
            send_dali_cmd(&btn->command);
        }
        break;

        case BTN_STATE_DEBOUNCING:
        if (is_timer_expired(btn)) {
            if (val == 0) {
                // It remains pressed.. Halleluja
                // TODO get target status...
                log_uint8("Pressed", index);
                btn->state = BTN_STATE_PRESSED;
                btn->next_action = RTC.CNT + config->doublePressTimer;
            } else {
                // it may have been just noise.  Ignore it. 
                btn->state = BTN_STATE_RELEASED;
            }
        }
        break;

        case BTN_STATE_RELEASED_DEBOUNCING:
        if (is_timer_expired(btn)) {
            btn->state = BTN_STATE_RELEASED;
        }
        break;


        case BTN_STATE_PRESSED:
        if (val) {
            log_uint8("Released", index);
            // Button has been released.
            if (btn->command.result == READ_VALUE) {
                log_uint8("Old value ", btn->command.output);
                btn->command.cmd = config->targets[index] << 8 | (btn->command.output ? DALI_CMD_OFF : DALI_CMD_GO_TO_LAST_ACTIVE_LEVEL);
                send_dali_cmd(&btn->command);
            } else {
                log_uint8("Invalid command status", btn->command.result);
            }
        }
        // Fall through into setting up debounce.


        case BTN_STATE_LONGHELD:
        if (val) {
            // It was released.
            btn->state = BTN_STATE_RELEASED_DEBOUNCING;
            btn->next_action = RTC.CNT + config->repeatTimer;

        } else if (is_timer_expired(btn)) {
            // Its been held long enough now
            btn->next_action = RTC.CNT + config->repeatTimer; 
            if (btn->state == BTN_STATE_PRESSED) {
                log_uint8("Long Held", index);
                btn->state = BTN_STATE_LONGHELD;
            } else {
                log_uint8("Repeat", index);
            }
        }
        break;
    }
}


bool poll_buttons() {
    bool all_idle = true;

    for (uint8_t b = 0; b < NUM_BUTTONS; b++) {
        poll_button(b, &buttons[b]);
        // If any button is doing anything other than in the released state, then we stay awake.
        if (buttons[b].state != BTN_STATE_RELEASED) {
            all_idle = false;
        }
    }
    return all_idle;
}
