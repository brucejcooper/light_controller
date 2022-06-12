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
} dali_result_t;


extern void dali_init();
extern dali_result_t dali_transmit_cmd(uint8_t addr, uint8_t cmd);
extern void dali_debug_blink();

#endif