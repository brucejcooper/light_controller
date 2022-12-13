#ifndef __OUTGOING_COMMANDS_H__
#define __OUTGOING_COMMANDS_H__

#include "snd.h"

typedef enum  {
    COMMAND_TURN_OFF = 0x00,
    COMMAND_TURN_ON = 0x05,
} outgoing_command_t;

typedef enum  {
    COMMAND_RESPONSE_NACK,
    COMMAND_RESPONSE_VALUE,
    COMMAND_RESPONSE_ERROR,
} outgoing_command_response_type_t;


typedef void (*response_handler_t)(outgoing_command_response_type_t responseType, uint8_t response);

void transmitCommand(uint16_t val, response_handler_t responseHandler);

#endif