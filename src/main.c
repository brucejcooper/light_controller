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


// Set up to allow a fair amount of processing, including dimming where several 
// 20ms commands are chained.
#define NORMAL_WDT PERIOD_256CLK_gc


void reset() {
    // Write a bit into the SWRR to reboot the device (to the bootloader).
    RSTCTRL.SWRR = RSTCTRL_SWRE_bm;
}

static inline void set_wdt(uint8_t val) {
    while (WDT.STATUS & WDT_SYNCBUSY_bm) {
        ;
    }
    CCP = CCP_IOREG_gc;
    WDT.CTRLA = val;
}

int main(void) {
    set_wdt(NORMAL_WDT); 
    console_init();

    // Set the DALI output (PB2) as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Set up the DALI input (PA7) using the Analog Comparator with reference of 0.55V
    // This makes it trigger sooner than if we were doing digital I/O, as it has a much lower threshold
    VREF.CTRLA = VREF_DAC0REFSEL_0V55_gc;
    PORTA.PIN7CTRL  = PORT_ISC_INPUT_DISABLE_gc; // Disable Digital I/O
    AC0.MUXCTRLA = AC_MUXNEG_VREF_gc | AC_MUXPOS_PIN0_gc;
    AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm; // Enable the AC.    

    // Turn on the RTC and PIT
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV2_gc;
    RTC.PITCTRLA = RTC_PERIOD_CYC16384_gc | RTC_PITEN_bm; // Seems to go off 1/sec no matter what the prescaler is.
    RTC.PITINTCTRL = RTC_PI_bm;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    buttons_init();
    sei();

    while (1) {
        if (poll_buttons()) {            
            // Clear any interrupts that may cause the device to wake up immediately
            console_flush();
            _delay_us(64);
            PORTA.INTFLAGS = PIN6_bm;
            RTC.PITINTFLAGS = RTC_PI_bm;
            // Change the WDT to at least twice the PIT while we sleep
            set_wdt(WDT_PERIOD_2KCLK_gc);
            sleep_mode();
            set_wdt(NORMAL_WDT); // Restore the WDT.
        }
    }
    return 0;
}