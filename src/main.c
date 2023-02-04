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
    // set_wdt(NORMAL_WDT); 
    set_wdt(WDT_PERIOD_8KCLK_gc);
    console_init();
    log_uint8("Reset Reason", RSTCTRL_RSTFR);
    RSTCTRL_RSTFR = 0XFF;
    log_uint16("Long press timeout", config->doublePressTimer);
    log_uint16("Dim repeat timer", config->repeatTimer);

    // Set the DALI output (PB2) as an output, initially set to zero out (not shorted)
    PORTB.OUTCLR = PORT_INT2_bm;
    PORTB.DIRSET = PORT_INT2_bm;

    // Set up the DALI input (PA7) using the Analog Comparator with reference of 0.55V
    // This makes it trigger sooner than if we were doing digital I/O, as it has a much lower threshold
    VREF.CTRLA = VREF_DAC0REFSEL_0V55_gc;
    PORTA.PIN7CTRL  = PORT_ISC_INPUT_DISABLE_gc; // Disable Digital I/O, so that it doesn't mess with the impedence
    AC0.MUXCTRLA = AC_MUXNEG_VREF_gc | AC_MUXPOS_PIN0_gc;
    AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm; // Enable the AC.    


    // Turn on the RTC and PIT
    RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; // Slow down buddy.
    RTC.PITCTRLA = RTC_PERIOD_CYC4096_gc | RTC_PITEN_bm; // Set the PIT to go off at least once per maximum WDT period.
    while (RTC.STATUS & RTC_CTRLABUSY_bm) {
        ;
    }
    RTC.CTRLA = RTC_RTCEN_bm | RTC_PRESCALER_DIV1_gc;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    buttons_init();
    sei();

    uint8_t loop = 0;

    while (1) {
        if (poll_buttons()) {            
            log_uint8("S", ++loop);
            console_flush();
            // Enable interrupts to wake us back up
            PORTA.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_LEVEL_gc;
            RTC.PITINTCTRL = RTC_PI_bm;
            sleep_mode();            
            // Disable the interrupts again, as everything is syncrhonous.
            RTC.PITINTCTRL = 0;
            PORTA.PIN6CTRL = PORT_PULLUPEN_bm; 
        }
    }
    return 0;
}

// we don't expect the PIT to do anything except wake us up, but if there isn't an ISR,
// odd things happen
ISR(RTC_PIT_vect)
{
    RTC.PITINTFLAGS = RTC_PI_bm;
}

// Likewise, the PORTA interrupt is only there to wake us up.  Immediately disable the 
// interrupt before clearing.
ISR(PORTA_PORT_vect)
{
    log_uint8("PA", PORTA.INTFLAGS);
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
    PORTA.INTFLAGS = PIN6_bm;
}