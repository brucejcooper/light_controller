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


typedef struct timer_t {
    timer_handler_t handler;
    uint16_t timeout;
    timer_t *next;
} timer_t;

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
 * @brief this should go off when a short timer has expired
 * 
 */
ISR(RTC_CNT_vect) {
    while (RTC.STATUS & RTC_CNTBUSY_bm) {
        ;
    }
    uint16_t now = RTC.CNT;
    // Check each timer to see if it has expired.
    bool allIdle = true;
    for (int8_t i = 0; i < MAX_TIMERS; i++) {
        if (timers[i].handler) {
            int16_t ttf = timeTillFire(now, timers[i].timeout);
            if (ttf <= 0) {
                timers[i].handler(i);
                timers[i].handler = NULL; // All timers are single shot.
            } else {
                allIdle = false;
            }
        }
    }
    if (allIdle) {
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
    if (long_timer.handler && long_timer.timeout && --long_timer.timeout == 0) {
        long_timer.handler(0);
    }
    disable_pit();
    // Clear the ISR
    RTC.PITINTFLAGS = RTC_PI_bm;
}


/**
 * @brief will actually fire up to one second before the value supplied
 * 
 * @param seconds 
 * @param handler 
 */
void setLongTimer(uint16_t seconds, timer_handler_t handler) {
    long_timer.handler = handler;
    long_timer.timeout = seconds;
    if (handler) {
        enable_pit();
    } else {
        disable_pit();
    }
}

void cancelTimer(timer_t_timer) {
    
}

timer_t *createTimer(uint16_t period, timer_handler_t handler) {
    timer_t *timer = malloc(sizeof(timer_t));
    if (!timer) {
        return timer;
    
        while (RTC.STATUS & RTC_CNTBUSY_bm) {
            ;
        }
        uint16_t now = RTC.CNT;
        timer->handler = handler;
        timer->timeout = now+period;

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
            // It should be at the head (t is implicitly timers).
            timers = timer;
        }
        enable_rtc();
    }
    return timer;
}



void initialise_timers() {
    // Configure RTC clock.
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // Use 1.024 Khz clock divided by 2 with the prescaler
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    for (uint8_t i = 0; i < MAX_TIMERS; i++) {
        timers[i].handler = NULL;
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
