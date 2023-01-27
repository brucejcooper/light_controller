#ifndef __INCOMING_COMMANDS_H__
#define __INCOMING_COMMANDS_H__

#include "sched_callback.h"
#include "rcv.h"
#include "snd.h"

typedef enum {
    ADDRESS_TYPE_NOT_FOR_ME,
    ADDRESS_TYPE_EVENT,
    ADDRESS_TYPE_SPECIAL_DEVICE_COMMAND,
    ADDRESS_TYPE_SPECIAL_DEVICE_COMMAND_TWOARGS,

    ADDRESS_TYPE_DEVICE_INSTANCE,
    ADDRESS_TYPE_INSTANCE_GROUP,
    ADDRESS_TYPE_INSTANCE_TYPE,
    ADDRESS_TYPE_FEATURE_INSTANCE_NUMBER,
    ADDRESS_TYPE_FEATURE_INSTANCE_GROUP,
    ADDRESS_TYPE_FEATURE_INSTANCE_TYPE,
    ADDRESS_TYPE_FEATURE_INSTANCE_BROADCAST,
    ADDRESS_TYPE_INSTANCE_BROADCAST,
    ADDRESS_TYPE_FEATURE_DEVICE,
    ADDRESS_TYPE_DEVICE,
} address_type_t;

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
    uint8_t repeatCount;
    address_type_t addressing;

    union {
        receive_event_t other;
        my_response_t mine;
    } response;
} command_event_t;


void wait_for_command();
void setCommandObserver(user_mode_callback_t cb);

#endif