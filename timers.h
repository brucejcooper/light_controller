/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

#ifndef __TIMERS_H__
#define __TIMERS_H__

#include <inttypes.h>
#include <stdbool.h>

typedef void (*pulsewidth_handler_t)(uint16_t pulse_width);

void update_timeout(uint16_t to);
void on_timeout(uint16_t timeout, void (*callback)());
void start_timeout();
void cancel_timeout();
void reset_timeout();

#endif