/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

#ifndef __DALI_H__
#define __DALI_H__

#include <inttypes.h>
#include <stdbool.h>

typedef enum { 
    DALI_OK = 0, 
    DALI_COLLISION_DETECTED,
    DALI_NO_START_BIT,
    DALI_NOT_FOR_ME,
    DALI_CORRUPT_READ,
} dali_result_t;


// Only correct for clock of 3.33Mhz
#define TICKS_TO_USEC(t)    (t*3/10)
#define USEC_TO_TICKS(u)    (u*10/3)

#define DALI_BIT_USECS              (833)
#define DALI_HALF_BIT_USECS         (DALI_BIT_USECS/2)
#define DALI_READ_TOLERANCE_USECS   (42)

#define HALFTICK_MIN_TICKS          USEC_TO_TICKS(DALI_HALF_BIT_USECS-DALI_READ_TOLERANCE_USECS)
#define HALFTICK_MAX_TICKS          USEC_TO_TICKS(DALI_HALF_BIT_USECS+DALI_READ_TOLERANCE_USECS)
#define FULLTICK_MIN_TICKS          USEC_TO_TICKS(DALI_BIT_USECS-DALI_READ_TOLERANCE_USECS)
#define FULLTICK_MAX_TICKS          USEC_TO_TICKS(DALI_BIT_USECS+DALI_READ_TOLERANCE_USECS)


// The DALI configuration - PA4 for transmit - PA3 for receive.
#define DALI_PORT       PORTA
#define DALI_RX_bm      PIN3_bm
#define DALI_TX_bm      PIN4_bm


extern void dali_init();
extern dali_result_t dali_transmit_cmd(uint8_t addr, uint8_t cmd);
extern void dali_debug_blink();
extern dali_result_t dali_receive(uint8_t *address, uint8_t *command);


#endif