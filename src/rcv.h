#ifndef __RCV_H__
#define __RCV_H__

#include <stdint.h>

typedef enum {
    RECEIVE_EVT_RECEIVED,
    RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD,
    RECEIVE_EVT_INVALID_SEQUENCE,
} receive_event_type_t;

typedef struct {
    receive_event_type_t type;
    uint32_t data;
    uint8_t numBits;
} receive_event_received_t;

typedef union {
    receive_event_type_t type;
    receive_event_received_t rcv;
} receive_event_t;




typedef void (*receive_cb_t)(receive_event_t *evt);
void waitForRead(uint16_t timeout, receive_cb_t cb);


#endif