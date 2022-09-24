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
#include "console.h"
#include "timers.h"

// TODO What is the correct value here?
#define IDLE_THRESHOLD USEC_TO_TICKS(19000)



/**
 * @brief Called when the timeout occurs (only enabled when line is released).  
 * This is good. It means that the line has been idle for enough time. 
 */
static void onTimeout() {
    // Turn everything off.
    cancel_timeout();
    dali_on_linechange(NULL);
    dali_idle_state_enter(); 
}


static void startReceivingResponse() {
    if (dali_is_bus_shorted()) {
        // Disable the timer. We will test again next time we get a toggle ISR.
        cancel_timeout();
    } else {
        // Set up a timer to go off if we don't receive a positive edge within the threshold period.
        start_timeout();
    }
}


extern void dali_wait_for_idle_state_enter() {
    USART0_sendChar('C');
    dali_on_linechange(startReceivingResponse);
    on_timeout(IDLE_THRESHOLD, onTimeout);
    startReceivingResponse();
}