#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <stdbool.h>
#include <string.h>
#include "buttons.h"
#include "config.h"
#include <stdlib.h>
#include "cmd.h"


// For test board, switch is on PA6
#define SWITCH_PORT PORTA
#define MS_TO_RTC_TICKS(m) (m * 1024 / 1000)
struct button_t;

typedef enum {
    BTN_STATE_RELEASED,
    BTN_STATE_DEBOUNCING,
    BTN_STATE_PRESSED,
    BTN_STATE_LONGHELD,
    BTN_STATE_RELEASE_DEBOUNCE,
    BTN_STATE_RELEASED_WAIT_FOR_REPRESS,
} button_state_t; 


typedef enum {
    SLEEP_STATE_PROCESSING,
    SLEEP_STATE_WAITING,
    SLEEP_STATE_SLEEP_READY
} sleep_state_t;


static sleep_state_t sleepState = SLEEP_STATE_PROCESSING;
static uint16_t idleTimeout;


typedef void (*state_handler_t)(struct button_t *btn, const uint8_t button_level);
typedef struct button_t {
    // Index of the button.
    uint8_t index;

    // Dimming Direction.
    dali_gear_command_t direction;

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
static void wait_for_repress(button_t *btn, const uint8_t button_level);
static void long_pressed(button_t *btn, const uint8_t button_level);

static button_t buttons[NUM_BUTTONS] = {
    {
        .index = 0,
        .state = BTN_STATE_RELEASED,
        .mask = PIN6_bm,
        .light_level = 0,
        .direction = DALI_CMD_DOWN,
        .timeout = 0,
    }
};


void buttons_init() {
    // Set up pins with pullup, with no interrupt.  the interrupt will be turned on while sleeping.
    SWITCH_PORT.PIN6CTRL = PORT_PULLUPEN_bm;

    // Set all the pins as inputs.
    SWITCH_PORT.DIRCLR = PIN6_bm;
}

static inline bool check_timeout(uint16_t t) {
    return ((int16_t) (RTC.CNT - t)) >= 0;
}

static inline bool is_timer_expired(button_t *btn) {
    return check_timeout(btn->timeout);
}


static void send_dali_cmd_no_response(button_t *btn, dali_gear_command_t cmd) {
    uint8_t dummy;
    read_result_t res = send_dali_cmd(config->targets[btn->index], cmd, &dummy);
    if (res != READ_NAK) {
        // What do we do?
    }
}

static uint8_t send_dali_query(button_t *btn, dali_gear_command_t cmd, uint8_t defaultVal) {
    uint8_t val;
    read_result_t res = send_dali_cmd(config->targets[btn->index], cmd, &val);
    if (res != READ_VALUE) {
        return defaultVal;
    }
    return val;    
}




static inline void execute_dim(button_t *btn) {
    send_dali_cmd_no_response(btn,  btn->direction);
}


static inline void do_press(button_t *btn) {
    // Its been pressed.
    btn->state = BTN_STATE_DEBOUNCING;
    btn->timeout = RTC.CNT + MS_TO_RTC_TICKS(20);
}

static void debouncing(button_t *btn, const uint8_t button_level) {
    if (button_level) {
        // Bounce!  We ignore small pulses (less than the debounce timer) to be noise resistant.
        btn->state = BTN_STATE_RELEASED;
    } else if (is_timer_expired(btn)) {
        // Graduated to pressed. 
        btn->state = BTN_STATE_PRESSED;
        btn->timeout = RTC.CNT + config->doublePressTimer;

        // ask the ballast its current level.
        // this takes some time (15-20 ms, including post response delay)
        btn->light_level = send_dali_query(btn, DALI_CMD_QUERY_ACTUAL_LEVEL, 0);     
    }
}


static void released(button_t *btn, const uint8_t button_level) {
    if (!button_level) {
        do_press(btn);
    }
}

static void pressed(button_t *btn, const uint8_t button_level) {
    if (button_level) {
        // Its been released - send out either an off or an on command, depending ont he current level
        send_dali_cmd_no_response(btn, btn->light_level ? DALI_CMD_OFF : DALI_CMD_GO_TO_LAST_ACTIVE_LEVEL);
        // The command will have taken a little over 10 milliseconds.  This is effectively our debounce period.
        btn->state = BTN_STATE_RELEASED;
    } else if (is_timer_expired(btn)) {
        btn->state = BTN_STATE_LONGHELD;
        btn->timeout = RTC.CNT + config->repeatTimer;
        if (btn->light_level == 0) {
            // We can't dim or brighten if we're not on, so turn it on, and find out what the current level is.
            send_dali_cmd_no_response(btn, DALI_CMD_GO_TO_LAST_ACTIVE_LEVEL);
            btn->light_level = send_dali_query(btn, DALI_CMD_QUERY_ACTUAL_LEVEL, 0);     
        }
        // TODO is there a way this can be hidden something else?  Perhaps during the long press delay?
        uint8_t minLevel = send_dali_query(btn, DALI_CMD_QUERY_MIN_LEVEL, 0);
        // If we're already at minimum, start out brightening, otherwise start dimming
        btn->direction = btn->light_level <= minLevel ? DALI_CMD_UP : DALI_CMD_DOWN;
        execute_dim(btn);
    }
}

static void long_pressed(button_t *btn, const uint8_t button_level) {
    if (button_level) {
        // It was released. Do some debouncing.
        btn->state = BTN_STATE_RELEASE_DEBOUNCE;
        btn->timeout = RTC.CNT + MS_TO_RTC_TICKS(10); 
    } else if (is_timer_expired(btn)) {
        // Its been held long enough now for a repeat.
        btn->timeout = RTC.CNT + config->repeatTimer; 
        execute_dim(btn);
    }
}

static void release_debounce(button_t *btn, const uint8_t button_level) {
    if (is_timer_expired(btn)) {
        btn->state = BTN_STATE_RELEASED_WAIT_FOR_REPRESS;
        btn->timeout = RTC.CNT + config->repeatTimer;
    }
}

static void wait_for_repress(button_t *btn, const uint8_t button_level) {
    if (!button_level) {
        // Its a repress (Kinda like a double click, but after a long hold)
        // TODO If you immediately repress, I wonder if going directly to max (or min) would be a good idea.  An easy way of getting to an extreme without having to wait. 
        btn->direction = btn->direction == DALI_CMD_UP ? DALI_CMD_DOWN : DALI_CMD_UP;
        btn->timeout = RTC.CNT + config->repeatTimer; 
        btn->state = BTN_STATE_LONGHELD;
        execute_dim(btn);
    } else if (is_timer_expired(btn)) {
        btn->state = BTN_STATE_RELEASED;
    }
}


static void poll_button(button_t *btn, uint8_t val) {
    switch (btn->state) {
        case BTN_STATE_RELEASED:
            released(btn, val);
            break;
        case BTN_STATE_DEBOUNCING:
            debouncing(btn, val);
            break;
        case BTN_STATE_PRESSED:
            pressed(btn, val);
            break;
        case BTN_STATE_LONGHELD:
            long_pressed(btn, val);
            break;
        case BTN_STATE_RELEASE_DEBOUNCE:
            release_debounce(btn, val);
            break;
        case BTN_STATE_RELEASED_WAIT_FOR_REPRESS:
            wait_for_repress(btn, val);
            break;
        default:
            // Illegal state.
            btn->state = BTN_STATE_RELEASED;
            break;
    }
}



bool poll_buttons() {
    bool all_idle = true;

    for  (button_t *btn = buttons; btn < (buttons+NUM_BUTTONS); btn++) {
        uint8_t val = SWITCH_PORT.IN & btn->mask;
        poll_button(btn, val);
        if (btn->state != BTN_STATE_RELEASED) {
            all_idle = false;
        }
        wdt_reset();
    }
    // If nothing happens for 1/2 second once we return to an all-idle state, sleep.
    // This is needed to deal with debouncing during press.
    if (all_idle) {
        switch (sleepState) {
            case SLEEP_STATE_SLEEP_READY:
                return true;

            case SLEEP_STATE_WAITING:
                if (check_timeout(idleTimeout)) {
                    sleepState = SLEEP_STATE_SLEEP_READY;
                }
            break;

            case SLEEP_STATE_PROCESSING:
                sleepState = SLEEP_STATE_WAITING;
                idleTimeout = RTC.CNT + MS_TO_RTC_TICKS(500); 
            break;
        }
    } else {
        sleepState = SLEEP_STATE_PROCESSING;
    }
    return false;
}


// This is called if a button was pressed while sleeping (the normal entry to a button being pressed)
ISR(PORTA_PORT_vect) {
    // Turn off all interrupts, as we're going back to polling.
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;

    // We know the button was pressed, and that all buttons must have been idle beforehand.
    uint8_t processed = 0x00;
    for  (button_t *btn = buttons; btn < (buttons+NUM_BUTTONS); btn++) {
        if (PORTA.INTFLAGS & btn->mask) {
            processed |= btn->mask;
            do_press(btn);
        }
    }
    // Acknowledge the processed interrupts. 
    PORTA.INTFLAGS = processed;
}