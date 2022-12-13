#ifndef __SND_H__
#define __SND_H__

#include "stdint.h"

typedef enum {
    TRANSMIT_EVT_COMPLETED,
    TRANSMIT_EVT_COLLISION
} transmit_event_t;

typedef void (*transmit_cb_t)(transmit_event_t event_type);


void transmit(uint32_t val, uint8_t len, transmit_cb_t callback);


#endif