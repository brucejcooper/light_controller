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
#include "intr.h"
#include "timing.h"
#include "cmd.h"

typedef void (*isr_pulse_handler_t)(uint16_t);

static uint8_t lastBit;
static volatile uint8_t numBits;
volatile bool receive_idle = true;

static volatile cmd_queue_entry_t *current_tx;

static void pulse_after_half_bit(uint16_t pulseWidth);
static void pulse_after_bit_boundary(uint16_t pulseWidth);
static void startbit_started(uint16_t _ignored);
static isr_pulse_handler_t tcb0_handler = NULL;


uint16_t invalidPulseWidth;



static inline void set_input_pulse_handler(isr_pulse_handler_t tcb0) {
    tcb0_handler = tcb0;
}

// TODO this and the above are basically the same thing.  Refactor.
static void set_incoming_pulse_handler(isr_pulse_handler_t tcb0) {
    // clear any existing ISRS
    TCB0.INTFLAGS = TCB_CAPT_bm;
    tcb0_handler = tcb0;
    TCB0.INTCTRL = tcb0_handler ? TCB_CAPT_bm : 0;
}



static inline void pushBit() {
    log_char(lastBit ? '1': '0');
    current_tx->output = current_tx->output << 1 | lastBit;
    numBits++;
}


static void pulse_after_invalid(uint16_t pulseWidth) {
    log_char('^');
    // All subsequent pulses until timeout are ignored. 
}

static void invalid_sequence_received() {
    log_char('!');
    current_tx->result = READ_COLLISION;
    set_input_pulse_handler(pulse_after_invalid);
}

static void pulse_after_start(uint16_t pulseWidth) {
    if (!isHalfBit(pulseWidth)) {
        invalidPulseWidth = pulseWidth; // Record the bad pulse into the data so it can be logged.  Bit position will also be relevant.
        invalid_sequence_received();
    } else {
        log_char('S');
        lastBit = 1;
        set_input_pulse_handler(pulse_after_half_bit);
    }
}


static void pulse_after_half_bit(uint16_t pulseWidth) {
    if (isHalfBit(pulseWidth)) {
        log_char('h');
        set_input_pulse_handler(pulse_after_bit_boundary);
    } else if (isFullBit(pulseWidth)) {
        log_char('f');
        lastBit = lastBit ? 0 : 1;
        pushBit();
    } else {
        invalidPulseWidth = pulseWidth; // Record the bad pulse into the data so it can be logged.  Bit position will also be relevant.
        invalid_sequence_received();
    }
}

static void pulse_after_bit_boundary(uint16_t pulseWidth) {
    if (!isHalfBit(pulseWidth)) {
        invalidPulseWidth = pulseWidth; // Record the bad pulse into the data so it can be logged.  Bit position will also be relevant.
        invalid_sequence_received();
    } else {
        log_char('H');
        pushBit();
        set_input_pulse_handler(pulse_after_half_bit);
    }
}


static void timeout_occurred() {
    log_char('t');
    log_char('\n');
    clearTimeout(); 
    TCB0.CTRLA = 0; // Stop TCB0
    TCB0.CTRLB = TCB_CNTMODE_INT_gc; // Turn off frequency capture.
    TCB0.INTCTRL = 0; // No more interrupts.
    TCB0.INTFLAGS = TCB_CAPT_bm;

    set_incoming_pulse_handler(NULL); // Turn off all ISRs.
    log_uint8("TOS", AC0.STATUS);
    if (!(AC0.STATUS & AC_STATE_bm) || numBits != 8) {
        log_uint8("NumBits", numBits);
        log_uint16("Invalid pulse width", TICKS_TO_USECS(invalidPulseWidth));

        // Timeout occurred while shorted.  This shouldn't happen.
        // shiftReg = 65535; // Magic value for debug.
        current_tx->result = READ_COLLISION;
    }
    // AC0.CTRLA = AC_HYSMODE_OFF_gc;  // Disable AC.

    receive_idle = true;
}


static inline void write_one() {
    PORTB.OUTSET = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
    PORTB.OUTTGL = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
}

static inline void write_zero() {
    PORTB.OUTCLR = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
    PORTB.OUTTGL = PORT_INT2_bm;
    _delay_us(DALI_HALF_BIT_USECS);
}


static inline void write_byte(uint8_t byte) 
{
    for (uint8_t i = 0; i < 8; i++) {
        //write each bit 1 at a time.
        if (byte & 0x80) {
            write_one();
        } else {
            write_zero();
        }
        byte <<= 1;
    }
}

void simple_write(uint8_t addr, uint8_t cmd) {
    // Check that line is high (and has been for some time?)
    // for each bit, drive low then high, or vice versa.
    if (AC0.STATUS & AC_STATE_bm) {
        log_info("Bus is driven low");
    }

    // Write the start bit
    write_one();
    write_byte(addr);
    write_byte(cmd);
    
    PORTB.OUTCLR = PORT_INT2_bm;
}

typedef enum {
    PULSE_HALF,
    PULSE_FULL,
    INVALID
} pulse_t;


static pulse_t read_pulse() {
    uint8_t lvl = AC0.STATUS & AC_STATE_bm;
    uint16_t t = 0;
    while ((AC0.STATUS & AC_STATE_bm) == lvl) {
        t = TCA0.SINGLE.CNT;
        if (t > USEC_TO_TICKS(DALI_BIT_USECS + DALI_MARGIN_USECS)) {
            return INVALID;
        }
    }
    TCA0.SINGLE.CNT = 0;
    if (t < USEC_TO_TICKS(DALI_HALF_BIT_USECS - DALI_MARGIN_USECS)) {
        return INVALID;
    }
    if (t < USEC_TO_TICKS(DALI_HALF_BIT_USECS + DALI_MARGIN_USECS)) {
        return PULSE_HALF;
    }
    if (t < USEC_TO_TICKS(DALI_BIT_USECS - DALI_MARGIN_USECS)) {
        return INVALID;
    }
    return PULSE_FULL;
}

    
void simple_read(uint16_t timeout, cmd_queue_entry_t *out) {
        // disable interrupts. but why?
    // ensure line is high
    
    // Start TCA0, counting from 0, no interrupts, no nothing
    TCA0.SINGLE.CNT = 0;
    TCA0.SINGLE.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // start the clock

    // Wait for line to go low - record when this happened, then restart the clock
    while (AC0.STATUS & AC_STATE_bm) {
        if (TCA0.SINGLE.CNT >= timeout) {
            // Nothing received within timeout period.
            out->result = READ_NAK;
            goto cleanup;
        } 
    }
    TCA0.SINGLE.CNT = 0;


    // Read the first half of the start bit.
    if (read_pulse() != PULSE_HALF) {
        goto error;
    }

    // We're now half way through the start bit.  Start reading pulses
    uint8_t val = 0;
    uint8_t last = 1;
    for (uint8_t i = 0; i < 8; i++) {
        val <<= 1;
        switch (read_pulse()) {
            case PULSE_HALF:
                // We immediately expect another half pulse to take us back to the half bit, meaning we have a bit the same as the last one
                if (read_pulse() != PULSE_HALF) {
                    goto error;
                }
                break;
            case PULSE_FULL:
                // Its a bit flip
                last = last ? 1 : 0;
                break;
            default:
                goto error;
                break;
        }
        val |= last;
    }
    if (last == 0) {
        // We finished with a 0, which drives the line high, then low.  Read one last pulse in
        if (read_pulse() != PULSE_HALF) {
            goto error;
        }
    }
    
    // For cleanliness' sake, we expect the bus to remain high for 2 bit periods
    while (TCA0.SINGLE.CNT < USEC_TO_TICKS(DALI_BIT_USECS*2)) {
        if ((AC0.STATUS & AC_STATE_bm) == 0) {
            log_info("After read, bus should be high");
            goto error;
        }
    }
    out->result = READ_VALUE;
    out->output = val;

    goto cleanup;
error:
    out->result = READ_COLLISION;
    // Fall through to cleanup
cleanup:
    // Stop TCA0
    // re-enable interrupts
    TCA0.SINGLE.CTRLA = 0;
}

void read_dali(uint16_t timeout, cmd_queue_entry_t *out) {
    current_tx = out;
    out->output = 0;
    numBits = 0;
    out->result = READ_NAK;
    receive_idle = false;
    invalidPulseWidth = 0xFAFA;

    // Disable TCA0 initially.
    TCA0.SINGLE.CTRLA = 0;
    if (timeout > 0) {
        startSingleShotTimer(timeout, timeout_occurred);
    }

    set_incoming_pulse_handler(startbit_started);

    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_FRQ_gc; // Put it in Capture Frequency Measurement mode.
    TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_EDGE_bm; // Waiting for a negative edge to start with.
    TCB0.INTFLAGS = TCB_CAPT_bm; // Clear any existing interrupt.
    TCB0.INTCTRL = TCB_CAPT_bm; // Enable Interrupts on CAPTURE
    log_hex(AC0.STATUS);
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; // Start TCB0 for pulse width timing - First pulse should reset clock.
    // Event is insta-occurring after first read.
    
   
    // AC0.CTRLA = AC_HYSMODE_OFF_gc | AC_ENABLE_bm; // Enable the AC.
}

void startbit_started(uint16_t _ignored) {
    // setCanTransmitImmediately(false);
    log_char('s');
    current_tx->result = READ_VALUE;
    invalidPulseWidth = _ignored;

    // TCB0 should already be running, in Frequency Capture mode. 
    if (TCB0.CTRLA != (TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm) || TCB0.CTRLB != TCB_CNTMODE_FRQ_gc || TCB0.EVCTRL != (TCB_CAPTEI_bm)) {
        log_info("Entering read when not in the right mode.");
    //     log_uint8("CTRLA", TCB0.CTRLA);
    //     log_uint8("CTRLB", TCB0.CTRLB);
    //     log_uint8("EVCTRL", TCB0.EVCTRL);
    }
    set_incoming_pulse_handler(pulse_after_start);
    // Start a timer that will go off after the maximum wait time (2 Bit periods) to indicate we're done.
    startSingleShotTimer(USEC_TO_TICKS(2*DALI_BIT_USECS), timeout_occurred); // We are done when no pulse is received within 2 BIT periods.
}






ISR(TCB0_INT_vect) {
    uint16_t cnt = TCB0.CCMP; // This will clear the interrupt flag.
    // log_uint16("T", cnt);
    log_char(TCB0.EVCTRL & TCB_EDGE_bm ? 'F' : 'R');
    TCB0.EVCTRL ^= TCB_EDGE_bm;  // We always toggle the edge we're looking for. 
    TCA0.SINGLE.CNT = 0; // Reset Timeout clock

    if (tcb0_handler) {
        tcb0_handler(cnt);
    }
}
