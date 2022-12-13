#include "rcv.h"
#include "snd.h"
#include "outgoing_commands.h"
#include "console.h"
#include "timing.h"

static response_handler_t responseHandler;

static void responseReceived(receive_event_t *evt) {
    switch (evt->type) {
        case RECEIVE_EVT_RECEIVED:
            if (evt->rcv.numBits != 8) {
                responseHandler(COMMAND_RESPONSE_ERROR, 0);
            } else {
                responseHandler(COMMAND_RESPONSE_VALUE, (uint8_t) evt->rcv.data);
            }
            break;
        case RECEIVE_EVT_INVALID_SEQUENCE:
            responseHandler(COMMAND_RESPONSE_ERROR, 0);
            break;
        case RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD:
            responseHandler(COMMAND_RESPONSE_NACK, 0);
            break;
    }
}

static void commandTransmitted(transmit_event_t evt) {
    switch (evt) {
        case TRANSMIT_EVT_COLLISION:
            // TODO wait for idle line, then try again. Possibly with random delay.
            log_info("Collision while transmitting");
            // For the moment, just fall through and treat it like it succeeded.
            // break;
        case TRANSMIT_EVT_COMPLETED:
            // the command was sent successfully.  Wait for (potential) response.
            waitForRead(MSEC_TO_TICKS(10.5), responseReceived);
            break;
    }
}

void transmitCommand(uint16_t val, response_handler_t handler) {
    log_info("Sending command 0x%04x", val);
    responseHandler = handler;
    transmit(val, 16, commandTransmitted);
}
