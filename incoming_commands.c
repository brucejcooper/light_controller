#include "rcv.h"
#include <stdint.h>
#include <stdbool.h>
#include "idle.h"
#include "console.h"
#include "intr.h"
#include "timing.h"



/*
from https://onlinedocs.microchip.com/pr/GUID-0CDBB4BA-5972-4F58-98B2-3F0408F3E10B-en-US-1/index.html?GUID-DA5EBBA5-6A56-4135-AF78-FB1F780EF475
A DALI 2.0 forward frame contains the Start bit, followed by the address byte, up to two data bytes, 
and a Stop condition (see Figure 2). The DALI 2.0 24-bit forward frame, including the Start and Stop 
bits, lasts for 23.2 ms, or approximately 56 half-bit times, while the 16-bit forward frame lasts 
for 16.2 ms, or 39 half-bit periods. Once the control device completes the transmission of the frame, 
the control gear must begin to transmit the backward frame no sooner than 5.5 ms (approximately 14 half-bit 
times) and no later than 10.5 ms (approximately 25 half-bit periods). Once the backward frame has been 
received in its entirety, the control device must wait a minimum of 2.4 ms (approximately six half-bit periods) 
before transmitting the next forward frame (see Figure 3).
*/
static uint8_t response;

typedef enum {
    RESPONSE_RESPOND,
    RESPONSE_NACK,
    RESPONSE_IGNORE,
} response_type_t;

static response_type_t processCommand(receive_event_received_t *evt, uint8_t *response) {
    if (evt->numBits == 24) {
        log_info("Processing command 0x%08lx", evt->data);
        // By default, ignore all commands
        return RESPONSE_NACK;
    } else {
        // We only respond to 24 bit commands, as we are a controller.
        log_info("Ignoring 0x%08lx (%d bits)", evt->data, evt->numBits);
        return RESPONSE_IGNORE;
    }
}



static void transmit_response_completed() {
    startSingleShotTimer(MSEC_TO_TICKS(2.4), transmitNextCommandOrWaitForRead);
}

static void transmit_response() {
    log_info("Responding 0x%02x", response);
    // 3. at least 6 half bit periods (2.4ms) before processing a new command
    transmit(response, 8, transmit_response_completed);
}

static void responseFromOtherDeviceReceived(receive_event_t *evt) {
   switch (evt->type) {
        case RECEIVE_EVT_RECEIVED:
            if (evt->rcv.numBits == 8) {
                log_info("Other device response 0x%02x", (uint8_t) evt->rcv.data);
            } else {
                log_info("received illegal response length of %d", evt->rcv.numBits);
            }
            break;
        case RECEIVE_EVT_INVALID_SEQUENCE:
            log_info("Invalid DALI sequence received while waiting for response");
            break;
        case RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD:
            log_info("NACK");
            break;
    }
    transmitNextCommandOrWaitForRead();
}

static void commandReceived(receive_event_t *evt) {
    response_type_t outcome;
    switch (evt->type) {
        case RECEIVE_EVT_RECEIVED:
            outcome = processCommand(&(evt->rcv), &response);
            switch (outcome) {
                case RESPONSE_RESPOND:
                    // 1. Wait 5.5 ms (the minimum delay)
                    // 2. Then send the response (1 byte)
                    startSingleShotTimer(MSEC_TO_TICKS(5.5), transmit_response);
                    break;
                case RESPONSE_NACK:
                    // Wait for maximum response time.  Ignore any inputs during this time.  
                    log_info("(not) Responding NACK");
                    startSingleShotTimer(MSEC_TO_TICKS(10.5), transmitNextCommandOrWaitForRead);
                    break;
                case RESPONSE_IGNORE:
                    // Potentially read in a response from the other device.
                    waitForRead(MSEC_TO_TICKS(10.5), responseFromOtherDeviceReceived);
                    break;
            }
            break;
        case RECEIVE_EVT_INVALID_SEQUENCE:
            log_info("Invalid DALI sequence received");
            break;
        case RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD:
            log_info("Impossible timeout received");
            break;
    }
}

void wait_for_command() {
    waitForRead(0, commandReceived);
}

