
#include "cmd.h"
#include <string.h>
#include "timing.h"
#include "intr.h"
#include <util/delay.h>
#include <avr/io.h>

typedef void (*state_callback_t)(void);


static void timer_cb();
static void idle();
static void transmitting();
static void post_transmit_wait();
static void waiting_for_response();
static void cooling_off();




#define MAX_QUEUE_DEPTH 5
static cmd_queue_entry_t* queue[MAX_QUEUE_DEPTH];
static uint8_t dali_cmd_queue_depth = 0;
static state_callback_t current_state = idle;

static volatile bool timer_expired = true;



static void idle() {
    if (dali_cmd_queue_depth > 0) {
        // We're the first... we can kick off TX. 
        transmit(queue[0]->cmd, 16);
        current_state = transmitting;
    }
}


static void transmitting() {
    if (transmit_idle) {
        // log_uint8("TX complete", AC0.STATUS);
        // timer_expired = false;
        // current_state = post_transmit_wait;
        // startSingleShotTimer(MSEC_TO_TICKS(2), timer_cb);
        _delay_ms(4);
        post_transmit_wait();
    }
}

static void post_transmit_wait() {
    // if (timer_expired) {
        // log_uint8("Post TX wait complete", AC0.STATUS);

        // log_info("Transmit complete. Waiting for rx");
        // Done transmitting - start receiving (with timeout), for up to 22 Te or 9.17 milliseconds, minus the dead time.
        read_dali(USEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC-2), queue[0]);
        current_state = waiting_for_response;
    // }
}



static void waiting_for_response() {
    if (receive_idle) {
        // log_uint8("Receive complete", queue[0]->result);

        // Receive complete... Delete item from the queue
        if (--dali_cmd_queue_depth) {
            for (int i = 0; i < dali_cmd_queue_depth; i++) {
                queue[i] = queue[i+1];
            }
        }
        // before we can start transmitting, we need to wait for another 22 Te
        timer_expired = false;
        current_state = cooling_off;
        startSingleShotTimer(USEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), timer_cb);
    }

}

static void cooling_off() {
    if (timer_expired) {
        current_state = idle;
        current_state(); // Immediately check if we need to go into transmitting.
    }
}

/**
 * @brief Called once the timer goes off, in the ISR. 
 * 
 */
static void timer_cb() {
    timer_expired = true;
    process_dali_cmd_queue();
}


bool process_dali_cmd_queue() {
    current_state();
    return current_state == idle;
}


void send_dali_cmd(cmd_queue_entry_t *entry) {
    entry->result = READ_QUEUED;
    if (dali_cmd_queue_depth == MAX_QUEUE_DEPTH) {
        // TODO return error code.
        return;
    }
    queue[dali_cmd_queue_depth++] = entry;
    process_dali_cmd_queue();
}
