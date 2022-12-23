#include "state_machine.h"
#include <stdint.h>
#include <string.h>
#include "buttons.h"
#include "incoming_commands.h"
#include "outgoing_commands.h"
#include "console.h"
#include "config.h"

typedef struct {
    outgoing_command_t cmd;
    uint8_t index;
    response_context_handler_t callback;
    void *context;
} in_flight_command_t;


static bool canTransmitImmediately = false;
static in_flight_command_t commands[MAX_COMMANDS];
static uint8_t commandQueueDepth = 0;



static void commandResponseHandler(outgoing_command_response_type_t responseType, uint8_t response) {
    // If there is a callback, call it.
    if (commands[0].callback) {
        commands[0].callback(commands[0].cmd, commands[0].index, responseType, response, commands[0].context);
    }
    // Delete the queued command
    if (--commandQueueDepth > 0) {
        memcpy(commands, commands+1, commandQueueDepth*sizeof(in_flight_command_t));
    }
    // Check to see what to do next
    transmitNextCommandOrWaitForRead();
}

static void executeQueuedCommand() {
    canTransmitImmediately = false;
    // Index 0 is broadcast.
    uint8_t address = commands[0].index == 0 ? 0xFF : config.targets[commands[0].index-1];
    transmitCommand((address << 8) | commands[0].cmd, commandResponseHandler);
}

bool enqueueCommand(outgoing_command_t cmd, uint8_t index, response_context_handler_t callback, void *context) {
    if (commandQueueDepth == MAX_COMMANDS) {
        log_info("Not appending command, as queue is full");
        return false;
    }

    if (index > 5) {
        log_info("Illegal device index supplied");
        return false;
    }

    // Enqueue the command to transmit next time the device becomes idle. 
    in_flight_command_t *newCmd = commands + commandQueueDepth++;
    newCmd->cmd = cmd;
    newCmd->index = index;
    newCmd->callback = callback;
    newCmd->context = context;

    // If we're idle right now, start transmitting.
    if (canTransmitImmediately) {
        executeQueuedCommand();
    }
    return true;
}

void transmitNextCommandOrWaitForRead() {
    // If there is a pending outgoing command, send it
    if (commandQueueDepth > 0) {
        executeQueuedCommand();
    } else {
        // otherwise go into read mode.
        setCanTransmitImmediately(true);
        wait_for_command();
    }
}


void setCanTransmitImmediately(bool canTransmit) {
    canTransmitImmediately = canTransmit;
}
