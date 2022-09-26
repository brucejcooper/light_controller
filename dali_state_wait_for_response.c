
#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "console.h"
#include "timers.h"

// TODO What is the correct value here?
#define NORESPONSE_TIMEOUT USEC_TO_TICKS(10500)

static dali_response_callback_t responseCallback;

static inline void cleanup() {
    cancel_timeout();
    dali_on_linechange(NULL);
}


/**
 * @brief If we got no response within period, we go back to idle. 
 */
static void onTimeout() {
    // Turn everything off.
    cleanup();
    responseCallback(false, 0);
}

static void responseReceived(uint32_t data, uint8_t numBits) {
    cleanup();
    if (numBits != 8) {
        dali_wait_for_idle_state_enter();  // Bad response - Bail out.
    } else {
        responseCallback(true, data);
    }
}


static void startReceivingResponse() {
    dali_state_receiving_enter();
}


extern void dali_wait_for_response_state_enter(dali_response_callback_t callback) {
    USART0_sendChar('w');
    // For this, we will be reading the line, so we enable our AC, and make it interrupt on any change.
    dali_on_linechange(startReceivingResponse);
    responseCallback = callback;
    dali_state_receiving_prepare(responseReceived);
    on_timeout(NORESPONSE_TIMEOUT, onTimeout);
    startReceivingResponse();
}