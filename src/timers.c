#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "buttons.h"
#include "console.h"
#include "timers.h"



static timer_t *timers = NULL;

static timer_t long_timer = { .handler = NULL, .timeout = 0 };

/**
 * @brief this does some signed math magic to deal with wraparound of thr RTC counter.
 * this means that the longest timeout that can be put in is 32766ish, otherwise it will
 * insta-expire
 * 
 * @param cnt now
 * @param timeout when its supposed to happen 
 * @return int16_t if -ve, the event was in the past, if +ve it is in the future
 */
static inline int16_t timeTillFire(uint16_t cnt, uint16_t timeout) {
    return (int16_t) (timeout - cnt);
}


static inline void disable_rtc() {
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = 0;
}

static inline void enable_rtc() {
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV2_gc;
}


static inline void enable_pit() {
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) {
        ;
    }
    RTC.PITINTCTRL = RTC_PI_bm;
    RTC.PITCTRLA = RTC_PITEN_bm | RTC_PERIOD_CYC512_gc;
}

static inline void disable_pit() {
    RTC.PITINTCTRL = 0;
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) {
        ;
    }
    RTC.PITCTRLA = 0;
}


/**
 * @brief this should go off when the first short timer has expired
 * 
 */
ISR(RTC_CNT_vect) {
    while (RTC.STATUS & RTC_CNTBUSY_bm) {
        ;
    }
    uint16_t now = RTC.CNT;

    while (timers && timeTillFire(now, timers->timeout) < 0) {
        // Cut out the timer from the LL before calling the handler, as it might re-start
        // the timer and we want the strucutre of the LL to be correct.
        timer_t *t = timers;
        timer_handler_t h = timers->handler;
        void *ctx = timers->context;
        timers = timers->next;
        t->next = NULL;
        h(ctx);
    }
    if (timers) {
        RTC.CMP = timers->timeout;
    } else {
        disable_rtc();
    }

    // Clear the ISR
    RTC.INTFLAGS = RTC_CMP_bm;
}

/**
 * This goes off every 1 second, when there is an active long timer 
 * it is used for the provisioning timeout. once the counter reaches 0 we
 * call the handler and shit down the PIT
 */
ISR(RTC_PIT_vect) {
    // Clear the ISR
    RTC.PITINTFLAGS = RTC_PI_bm;
    if (long_timer.timeout && --long_timer.timeout == 0) {
        disable_pit(); // Call disable first, in case the handler creates a new one.
        timer_handler_t h = long_timer.handler;
        if (h) {
            long_timer.handler = NULL; // Just to be safe.
            h(long_timer.context);
        }
    }
}


/**
 * @brief will actually fire up to one second before the value supplied
 * 
 * @param seconds 
 * @param handler The handler to call after seconds seconds, or NULL to disable.
 */
void setLongTimer(uint16_t seconds, timer_handler_t handler, void *ctx) {
    long_timer.handler = handler;
    long_timer.timeout = seconds;
    long_timer.context = ctx;
    if (handler) {
        enable_pit();
    } else {
        disable_pit();
    }
}

void startTimer(uint16_t period, timer_handler_t handler, void *ctx, timer_t *timer) {
    cli();
    while (RTC.STATUS & RTC_CNTBUSY_bm) {
        ;
    }
    if (timer->handler) {
        // Its already started... cancel it first.
        cancelTimer(timer);
    }

    uint16_t now = RTC.CNT;
    timer->handler = handler;
    timer->timeout = now+period;
    timer->context = ctx;

    timer_t *prev = NULL;
    timer_t *t = timers;
    // Step through the list until we find one that occurs _after us.
    while (t && timer->timeout > t->timeout) {
        prev = t;
        t = t->next;
    }
    //  Insert ourselves before it.
    timer->next = t;
    if (prev) {
        prev->next = timer;
    } else {
        // It should be at the head (t is implicitly timers), and therefore the one next to fire.
        timers = timer;
        // change when CMP will fire.
        RTC.CMP = timer->timeout;
    }
    enable_rtc();
    sei();
}

void cancelTimer(timer_t *timer) {
    cli();
    timer_t *prev = NULL;
    timer_t *t = timers;
    // Find our node's predecessor (we don't do doubly linked, as our lists are always really short)
    while (t && t != timer) {
        prev = t;
        t = t->next;
    };
    if (t) {    
        if (prev) {
            prev->next = timer->next;
        } else {
            timers = timer->next;
            // this changes CMP;
            if (timers) {
                RTC.CMP = timers->timeout;
            } else {
                // No more timers. 
                disable_rtc();
            }
        }
    }
    timer->next = NULL;
    sei();
}



void initialise_timers() {
    // Configure RTC clock.
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // Use 1.024 Khz clock divided by 2 with the prescaler
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = 0;
    RTC.INTCTRL = RTC_CMP_bm;

    // make the pit run once per second (every 512 cycles i think, as it respects the prescaler)
    while (RTC.PITSTATUS & RTC_CTRLBUSY_bm) {
        ;
    }
    RTC.PITINTCTRL = RTC_PI_bm;
    RTC.PITCTRLA = 0;
}
