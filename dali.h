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
    DALI_ERR_COLLISION_DETECTED,
    DALI_ERR_INVALID_STATE,
    DALI_ERR_NO_START_BIT,
    DALI_ERR_NOT_FOR_ME,
    DALI_ERR_CORRUPT_READ,
    DALI_ERR_QUEUEFULL,
} dali_result_t;


typedef enum {
    RESPONSE,
    NO_RESPONSE,
    COLLISION,
} dali_command_result_t;


// Only correct for clock of 3.33Mhz
#define TICKS_TO_USEC(t)    (t*3/10)
// 19.86 ms is the highest value that this will support.
#define USEC_TO_TICKS(u)    ((uint16_t) (((float)u)*(F_CPU/1000000.0) + 0.5))

#define DALI_BAUD                   (1200)
#define DALI_BIT_USECS              (1000000.0/DALI_BAUD)
#define DALI_HALF_BIT_USECS         (DALI_BIT_USECS/2.0)
#define DALI_READ_TOLERANCE_USECS   (42)

#define HALFTICK_MIN_TICKS          USEC_TO_TICKS(DALI_HALF_BIT_USECS-DALI_READ_TOLERANCE_USECS)
#define HALFTICK_MAX_TICKS          USEC_TO_TICKS(DALI_HALF_BIT_USECS+DALI_READ_TOLERANCE_USECS)
#define FULLTICK_MIN_TICKS          USEC_TO_TICKS(DALI_BIT_USECS-DALI_READ_TOLERANCE_USECS)
#define FULLTICK_MAX_TICKS          USEC_TO_TICKS(DALI_BIT_USECS+DALI_READ_TOLERANCE_USECS)


// The DALI configuration - PA4 for transmit - PA3 for receive.
#define DALI_PORT       PORTA
#define DALI_RX_bm      PIN5_bm


typedef void (*dali_transmit_completed_callback_t)(bool collision);
typedef void (*dali_response_callback_t)(bool responseReceived, uint8_t response);

extern void dali_init();
uint8_t dali_is_bus_shorted();

dali_result_t dali_queue_transmit(uint32_t data, uint8_t numBits, dali_transmit_completed_callback_t responseCallback);


// State management - each of these functions lives in its own source file. 
void dali_idle_state_enter();
extern void dali_transmitting_state_enter(uint32_t data, uint8_t numBits, dali_transmit_completed_callback_t responseCallback);
void dali_wait_for_idle_state_enter();
void dali_receiving_state_enter();
void dali_waiting_for_response_state_enter();

void dali_on_linechange(void (*callback)());

// Receiving state.
void dali_state_receiving_prepare(void (*callback)(uint32_t data, uint8_t numBits));
void dali_state_receiving_enter();


void dali_wait_for_response_state_enter(dali_response_callback_t responseCallback);






#endif