#define __DELAY_BACKWARD_COMPATIBLE__

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>
#include <stdbool.h>
#include "dali.h"
#include "console.h"
#include "timers.h"
#include <stdlib.h>


typedef struct dali_command_t {
    uint32_t data;
    uint8_t numBits;
    dali_transmit_completed_callback_t responseHandler;
    struct dali_command_t *next;
} dali_command_t;


#define RESPONSE_DELAY USEC_TO_TICKS(10000)


static dali_command_t *commandQueueHead = NULL;
static bool idle;
static bool respondToCommand;
static uint8_t response;

void check_for_commands_to_send();


static void cleanup() {
    // Disable the AC0 Interrupt
    dali_on_linechange(NULL);
}

/**
 * @brief 
 * 
 * @param data 
 * @param numBits can be 16 or 24 - 16 bits are gear commands, while 24 is a control command.
 * @param responseHandler called once we're done, with the response (if any).  Also called if there is a collision
 * @return dali_result_t 
 */
dali_result_t dali_queue_transmit(uint32_t data, uint8_t numBits, dali_transmit_completed_callback_t responseHandler) {
    // TODO make the queue static
    dali_command_t *newCmd = malloc(sizeof(dali_command_t));
    if (!newCmd) {
        return DALI_ERR_QUEUEFULL;
    }
    newCmd->data = data;
    newCmd->numBits = numBits;
    newCmd->responseHandler = responseHandler;
    newCmd->next = NULL;

    dali_command_t *ptr = commandQueueHead;
    if (ptr == NULL) {
        commandQueueHead = newCmd;
    } else {
        // Walk to the end of the queue
        while (ptr->next) {
            ptr = ptr->next;
        }
        ptr->next = newCmd;
    }

    if (idle) {
        check_for_commands_to_send();
    }
    return DALI_OK;
}


void check_for_commands_to_send() {
    dali_command_t *cmd = commandQueueHead;

    if (cmd != NULL) {
        // A command has been enqueued. 
        commandQueueHead = commandQueueHead->next;
        idle = false;
        cleanup();
        dali_transmitting_state_enter(cmd->data, cmd->numBits, cmd->responseHandler);
        free(cmd);
    }
}


static void transmitComplete(bool conflict) {
    dali_idle_state_enter();
}

static void transmitResponse() {
    dali_transmitting_state_enter(response, 8, transmitComplete);
}


static void process_received_dali_transmission(uint32_t data, uint8_t numBits) {
    idle = false;

    cleanup();
    log_info("CMD[%d]%x", numBits, data);

    // By default we don't respond.
    respondToCommand = false;

    // We only respond to 24 bit 
    if (numBits == 24) {
        // Its a controller instruction.
        respondToCommand = true;
        response = 0x01;
    }
    
    // We wait for the response delay, then transmit.
    if (respondToCommand) {
        on_timeout(RESPONSE_DELAY, transmitResponse);
        start_timeout();
    } else {
        // Not for us, go back to idle.  If somebody else responds, we will read it in as an 8 bit command, which we 
        // will ignore. 
        dali_idle_state_enter();
    }
}

void daliCommandStarted() {
    // We've started receiving;
    cleanup();
    dali_state_receiving_start();
    dali_on_linechange(NULL);
}

void dali_idle_state_enter() {
    // If we've something to send, send it immediately
    if (commandQueueHead) {
        // Immediately start transmitting the next command.
        check_for_commands_to_send();
    } else {
        // Otherwise, wait for a transmission to be received, or button to be pressed.
        USART0_sendChar('I');
        idle = true;
        dali_state_receiving_prepare(process_received_dali_transmission);

        // Configure AC0 to interrupt on Falling Edge, notifying us - which we then use to transition to reading_state.
        dali_on_linechange(daliCommandStarted);
    }
}