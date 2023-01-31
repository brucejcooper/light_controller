#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/atomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"
#include "cmd.h"
#include "buttons.h"
#include "config.h"


int main(void) {
    console_init();

    // Set PB2 as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Use PA7 as an Analog Comparator, with reference of 0.55V
    // This makes it trigger sooner than if we were doing digital I/O, as it has a much lower threshold
    VREF.CTRLA = VREF_DAC0REFSEL_0V55_gc;
    PORTA.PIN7CTRL  = PORT_ISC_INPUT_DISABLE_gc; // Disable Digital I/O
    AC0.MUXCTRLA = AC_MUXNEG_VREF_gc | AC_MUXPOS_PIN0_gc;
    AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm; // Enable the AC.    
    // Turn on the RTC
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV2_gc;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    buttons_init();

    sei();

    while (1) {
        bool all_idle = poll_buttons();

        if (all_idle) {
            log_info("Sleeping");
            console_flush();
            // Clear any interrupts that may cause the device to wake up immediately
            _delay_us(400);
            PORTA.INTFLAGS = PIN6_bm;
            sleep_mode();
        }
    }
    return 0;
}