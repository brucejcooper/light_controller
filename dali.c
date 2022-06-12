/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

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

// The DALI configuration - PA4
#define DALI_bm     PIN4_bm
#define DALI_PORT   PORTA



volatile bool collision_detected = false;

static inline void drive_dali_low() {
    DALI_PORT.OUT &= ~DALI_bm;
}

static inline void release_dali_high() {
    DALI_PORT.OUT |= DALI_bm;
}


static inline void delay_half_bit() {
    _delay_us(416);
}


static inline void transmit_high_half_bit() {
    // TODO Check for collisions.
    release_dali_high();
    delay_half_bit();
}


static inline void transmit_low_half_bit() {
    // TODO Check for collisions.
    drive_dali_low();
    delay_half_bit();
}

static inline void transmit_start() {
    // TODO Check that line is not driven low for 8 ms.
    transmit_low_half_bit();
    transmit_high_half_bit();    
}

static inline void transmit_stop() {
    // release the bus (just in case)
    release_dali_high();

    // Wait for 2 full bit cycles.
    _delay_us(416*4);
}



void dali_init() {
    // Set up dali port as output.
    DALI_PORT.DIRSET = DALI_bm;
    release_dali_high();
}

void dali_debug_blink() {
    drive_dali_low();
    _delay_ms(200/6);
    release_dali_high();
}


dali_result_t dali_transmit_cmd(uint8_t addr, uint8_t cmd) {
    uint16_t shiftReg = addr << 8 | cmd;
    // Clear collision detected flag.
    collision_detected = false;

    transmit_start();
    for (int i = 0; i < 16 && !collision_detected; i++) {
        if (shiftReg & (1 << 15)) {
              transmit_low_half_bit();
              transmit_high_half_bit(); 
        } else {
            transmit_high_half_bit();    
            if (!collision_detected) {
                transmit_low_half_bit();
            }
        }
        shiftReg <<= 1;
    }
    if (!collision_detected) {
        transmit_stop();
    }

    if (collision_detected) {
        return DALI_COLLISION_DETECTED;
    } else {
        return DALI_OK;
    }
}
