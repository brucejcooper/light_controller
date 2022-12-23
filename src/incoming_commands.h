#ifndef __INCOMING_COMMANDS_H__
#define __INCOMING_COMMANDS_H__

#include "sched_callback.h"
#include "rcv.h"
#include "snd.h"

typedef enum {
    RESPONSE_RESPOND,
    RESPONSE_NACK,
    RESPONSE_IGNORE,
    RESPONSE_REPEAT,
    RESPONSE_INVALID_INPUT,
} response_type_t;

typedef struct {
    uint8_t val;
    transmit_event_t outcome;
} my_response_t;

typedef struct {
    uint32_t val;
    uint8_t len;
    response_type_t outcome;

    union {
        receive_event_t other;
        my_response_t mine;
    } response;
} command_event_t;


void wait_for_command();
void setCommandObserver(user_mode_callback_t cb);

#endif