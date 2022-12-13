#ifndef __IDLE_H__
#define __IDLE_H__

#include <stdbool.h>
#include "outgoing_commands.h"

#define MAX_COMMANDS 5

typedef void (*response_context_handler_t)(outgoing_command_t cmd, uint8_t index, outgoing_command_response_type_t responseType, uint8_t response, void *context);


bool enqueueCommand(outgoing_command_t cmd, uint8_t index, response_context_handler_t callback, void *context);
void transmitNextCommandOrWaitForRead();
void setCanTransmitImmediately(bool canTransmit);


#endif