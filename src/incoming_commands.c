#include "rcv.h"
#include <stdint.h>
#include <stdbool.h>
#include "state_machine.h"
#include "intr.h"
#include "timing.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include "sched_callback.h"
#include "incoming_commands.h"
#include "snd.h"
#include "console.h"

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



typedef enum {
    CMD_IdentifyDevice = 0x00, // Device Command	0x01 | destination	0xFE	0x00	2			
    CMD_ResetPowerCycleSeen = 0x01, // Device Command	0x01 | destination	0xFE	0x01	2			
    CMD_Reset = 0x10, // Device Command	0x01 | destination	0xFE	0x10	2			
    CMD_ResetMemoryBank = 0x11, // Device Command	0x01 | destination	0xFE	0x11	2	DTR0		
    CMD_SetShortAddress = 0x14, // Device Command	0x01 | destination	0xFE	0x14	2	DTR0		
    CMD_EnableWriteMemory = 0x15, // Device Command			0x15	2			
    CMD_EnableApplicationController = 0x16, // Device Command			0x16	2			
    CMD_DisableApplicationController = 0x17, // Device Command			0x17	2			
    CMD_SetOperatingMode = 0x18, // Device Command			0x18	2	DTR0		
    CMD_AddToDeviceGroupsZeroToFifteen = 0x19, // Device Command			0x19	2	DTR1, DTR2		
    CMD_AddToDeviceGroupsSixteenToThirtyOne = 0x1a, // Device Command			0x1a	2	DTR1, DTR2		
    CMD_RemoveFromDeviceGroupsZeroToFifteen = 0x1b, // Device Command			0x1b	2	DTR1, DTR2		
    CMD_RemoveFromDeviceGroupsSixteenToThirtyOne = 0x1c, // Device Command			0x1c	2	DTR1, DTR2		
    CMD_StartQuiescentMode = 0x1d, // Device Command			0x1d	2			
    CMD_StopQuiescentMode = 0x1e, // Device Command			0x1E	2			
    CMD_EnablePowerCycleNotification = 0x1f, // Device Command			0x1F	2			
    CMD_DisablePowerCycleNotification = 0x20, // Device Command			0x20	2			
    CMD_SavePersistentVariables = 0x21, // Device Command			0x21	2			
    CMD_QueryDeviceStatusResponse = 0x30, // Device Command			0x30			
    CMD_QueryApplicationControllerError = 0x31, // Device Command			0x31			NumericMask	
    CMD_QueryInputDeviceError = 0x32, // Device Command			0x32			NumericMask	
    CMD_QueryMissingShortAddress = 0x33, // Device Command			0x33			Y/N	
    CMD_QueryVersionNumber = 0x34, // Device Command			0x34			Numeric	
    CMD_QueryNumberOfInstances = 0x35, // Device Command			0x35			Numeric	
    CMD_QueryContentDTR0 = 0x36, // Device Command			0x36		DTR0	Numeric	
    CMD_QueryContentDTR1 = 0x37, // Device Command			0x37		DTR1	Numeric	
    CMD_QueryContentDTR2 = 0x38, // Device Command			0x38		DTR2	Numeric	
    CMD_QueryRandomAddressH = 0x39, // Device Command			0x39			Numeric	
    CMD_QueryRandomAddressM = 0x3a, // Device Command			0x3a			Numeric	
    CMD_QueryRandomAddressL = 0x3b, // Device Command			0x3b			Numeric	
    CMD_ReadMemoryLocation = 0x3c, // Device Command			0x3c			Numeric	
    CMD_QueryApplicationControlEnabled = 0x3d, // Device Command			0x3d			Y/N	
    CMD_QueryOperatingMode = 0x3e, // Device Command			0x3e			Numeric	
    CMD_QueryManufacturerSpecificMode = 0x3f, // Device Command			0x3f			Y/N	
    CMD_QueryQuiescentMode = 0x40, // Device Command			0x40			Numeric	
    CMD_QueryDeviceGroupsZeroToSeven = 0x41, // Device Command			0x41			Numeric	
    CMD_QueryDeviceGroupsEightToFifteen = 0x42, // Device Command			0x42			Numeric	
    CMD_QueryDeviceGroupsSixteenToTwentyThree = 0x43, // Device Command			0x43			Numeric	
    CMD_QueryDeviceGroupsTwentyFourToThirtyOne = 0x44, // Device Command			0x44			Numeric	
    CMD_QueryPowerCycleNotification = 0x45, // Device Command			0x45			Y/N	
    CMD_QueryDeviceCapabilities = 0x46, // Device Command			0x46			"        ""application controller present"",
    CMD_QueryExtendedVersionNumber = 0x47, // Device Command			0x47			Numeric	
    CMD_QueryResetState= 0x48, // Device Command			0x48			Numeric	
} device_command_t;

typedef enum {
    CMD_SetEventPriority = 0x61, // Instance Command	0x01 | destination (top two bits 0)	bFFFIIIII	0x61	2	DTR0		"F = Flags, I = Instance ID
    CMD_EnableInstance = 0x62, //	2			
    CMD_DisableInstance = 0x63, //	2			
    CMD_SetPrimaryInstanceGroup = 0x64, //	2	DTR0		
    CMD_SetInstanceGroup1 = 0x65, //	2	DTR0		
    CMD_SetInstanceGroup2 = 0x66, //	2	DTR0		
    CMD_SetEventScheme = 0x67, //	2	DTR0		
    CMD_SetEventFilter = 0x68, //	2	DTR0,DTR1,DTR2		
    CMD_QueryInstanceType = 0x80, //			Numeric	
    CMD_QueryResolution = 0x81, //			Numeric	
    CMD_QueryInstanceError = 0x82, //			Numeric	
    CMD_QueryInstanceStatus = 0x83, //			"        ""instance error"",
    CMD_QueryEventPriority = 0x84, //			Numeric	
    CMD_QueryInstanceEnabled = 0x86, //			Y/N	
    CMD_QueryPrimaryInstanceGroup = 0x88, //			Numeric	
    CMD_QueryInstanceGroup1 = 0x89, //			Numeric	
    CMD_QueryInstanceGroup2 = 0x8a, //			Numeric	
    CMD_QueryEventScheme = 0x8b, //			instance = 0 device = 1 device_instance = 2 device_group = 3 instance_group = 4"	
    CMD_QueryInputValue = 0x8c, //			Numeric	
    CMD_QueryInputValueLatch = 0x8d, //			Numeric	
    CMD_QueryFeatureType = 0x8e, //			Numeric	
    CMD_QueryNextFeatureType = 0x8f, //			Numeric	
    CMD_QueryEventFilterZeroToSeven = 0x90, //			Numeric	
    CMD_QueryEventFilterEightToFifteen = 0x91, //			Numeric	
    CMD_QueryEventFilterSixteenToTwentyThree = 0x92, //			Numeric	
} instance_command_t;

typedef enum {
    CMD_Terminate = 0x00, // Special Device Command	0xC1 (Top two bits set)	0x00	0x00				
    CMD_Initialise = 0x01, //	0x01	0x00	2			
    CMD_Randomise = 0x02, //	0x02	0x00	2			
    CMD_Compare = 0x03, //	0x03	0x00			Y/N	
    CMD_Withdraw = 0x04, //	0x04	0x00				
    CMD_SearchAddrH = 0x05, //	0x05	VAL				
    CMD_SearchAddrM = 0x06, //	0x06	VAL				
    CMD_SearchAddrL = 0x07, //	0x07	VAL				
    CMD_ProgramShortAddress = 0x08, //	0x08	VAL				
    CMD_VerifyShortAddress = 0x09, //	0x09	VAL				
    CMD_QueryShortAddress = 0x0a, //	0x0a				Numeric	
    CMD_WriteMemoryLocation = 0x20, //	0x20	VAL		DTR0,DTR1	Numeric	
    CMD_WriteMemoryLocationNoReply = 0x21, //	0x21	VAL				
    CMD_DTR0 = 0x30, //	0x30	VAL		DTR0		
    CMD_DTR1 = 0x31, //	0x31	VAL		DTR1		
    CMD_DTR2 = 0x32, //	0x32	VAL		DTR2		
    CMD_SendTestframe = 0x33, //	0x33			DTR0,DTR1,DTR2		
    // these are two-arg special commands, where the value is in the first byte.
    CMD_DirectWriteMemory = 0xC5, //   b11000101	VAL	VAL		DTR0,DTR1	Numeric	
    CMD_DTR1DTR0 = 0xC7, //   b11000111	VAL	VAL		DTR0,DTR1		
    CMD_DTR2DTR1 = 0xC9, //   b11001001	VAL	VAL		DTR1,DTR2		
} special_device_command_t;

typedef enum {
    EVT_ButtonReleased, // Pushbutton Event	b0AAAAAA0	b1IIII00	"0b00000000
    EVT_ButtonPressed, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00000001				Filter = 0b00000010
    EVT_ShortPress, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00000010				Filter = 0b00000100
    EVT_DoublePress, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00000101				Filter = 0b00001000
    EVT_LongPressStart, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00001001				Filter = 0b00010000
    EVT_LongPressRepeat, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00001011				Filter = 0b00100000
    EVT_LongPressStop, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00001100				Filter = 0b01000000
    EVT_ButtonFree, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00001110				Filter = 0b10000000
    EVT_ButtonStuck, // Pushbutton Event	b0AAAAAA0	b1IIII00	0b00001111				Filter = 0b10000000 (same as stuck)
} pushbutton_event_t;

typedef enum {
    CMD_SetShortTimer = 0x00, // Pushbutton Command	0x01 | destination	bFFFIIIII	0x00	2	DTR0		20ms increments
    CMD_SetDoubleTimer = 0x01, // Pushbutton Command			0x01	2	DTR0		
    CMD_SetRepeatTimer = 0x02, // Pushbutton Command			0x02	2	DTR0		
    CMD_SetStuckTimer = 0x03, // Pushbutton Command			0x03	2	DTR0		Increments of 1sec
    CMD_QueryShortTimer = 0x0a, // Pushbutton Command			0x0a			Numeric	
    CMD_QueryShortTimerMin = 0x0b, // Pushbutton Command			0x0b				
    CMD_QueryDoubleTimer = 0x0c, // Pushbutton Command			0x0c				
    CMD_QueryDoubleTimerMin = 0x0d, // Pushbutton Command			0x0d				
    CMD_QueryRepeatTimer = 0x0e, // Pushbutton Command			0x0e				
    CMD_QueryStuckTimer = 0x0f, // Pushbutton Command			0x0f				    
} pushbutton_command_t;


static user_mode_callback_t commandObserver = NULL;
static command_event_t currentCommand = {
    .outcome = RESPONSE_IGNORE,
};


void notifyCallCompletion() {
    if (commandObserver) {
        // As the callback will happen asynchronously, we make a copy of the current command..
        command_event_t *evt = malloc(sizeof(command_event_t));
        if (evt) {
            memcpy(evt, &currentCommand, sizeof(command_event_t));
        }
        schedule_call(commandObserver, evt);
    }
}



static response_type_t processSpecialDeviceCommand(special_device_command_t cmd, uint8_t param, uint8_t param2, uint8_t *response) {
    switch (cmd) {
        case CMD_Terminate:
        case CMD_Initialise:
        case CMD_Randomise:
        case CMD_Compare:
        case CMD_Withdraw:
        case CMD_SearchAddrH:
        case CMD_SearchAddrM:
        case CMD_SearchAddrL:
        case CMD_ProgramShortAddress:
        case CMD_VerifyShortAddress:
        case CMD_QueryShortAddress:
            break;
        case CMD_WriteMemoryLocation:
        case CMD_WriteMemoryLocationNoReply:
        case CMD_DTR0:
            config.dtr.bytes[0] = param;
            return RESPONSE_IGNORE; 
        case CMD_DTR1:
            config.dtr.bytes[1] = param;
            return RESPONSE_IGNORE; 
        case CMD_DTR2:
            config.dtr.bytes[2] = param;
            return RESPONSE_IGNORE; 
        case CMD_SendTestframe:
        case CMD_DirectWriteMemory:
        case CMD_DTR1DTR0:
        case CMD_DTR2DTR1:
            break;
    }
    return RESPONSE_IGNORE;
}

static response_type_t processDeviceCommand(device_command_t cmd,  uint8_t *response) {
    if (cmd <= CMD_SavePersistentVariables) {
        // It needs to be repeated.
        if (currentCommand.repeatCount != 2) {
            return RESPONSE_REPEAT;
        }
    }
    switch (cmd) {
        case CMD_QueryInputDeviceError:
            *response = 0;
            return RESPONSE_RESPOND;

        case CMD_QueryApplicationControllerError:
            *response = 0;
            return RESPONSE_RESPOND;

        case CMD_QueryInstanceType:
            *response = 1; // Pushbutton
            return RESPONSE_RESPOND;


        case CMD_StartQuiescentMode:
            config.mode = DEVMODE_QUIESCENT;
            return RESPONSE_NACK;
        case CMD_StopQuiescentMode:
            config.mode = DEVMODE_NORMAL;
            return RESPONSE_NACK;
    }
    return RESPONSE_IGNORE;
}


static address_type_t getAddressType(uint8_t cmd[3], uint8_t *p) {
    if ((cmd[0] & 0x01) == 0) {
        return ADDRESS_TYPE_EVENT;
    }

    if (cmd[0] & 0xC0) {
        return cmd[0] == 0xC1 ? ADDRESS_TYPE_SPECIAL_DEVICE_COMMAND : ADDRESS_TYPE_SPECIAL_DEVICE_COMMAND_TWOARGS;
    }

    uint8_t deviceId = cmd[0] >> 1;
    if(deviceId == 0x7F) {
        // Its addressed to all devices.
    } else if (deviceId == 0x7E) {
        // It only matches us if we don't have a short address.
        if (config.shortAddr != 0x40) {
            return ADDRESS_TYPE_NOT_FOR_ME;
        }
    } else {
        if (config.shortAddr == deviceId) {
            // It matches the device, so we need to check below. 
        } else {
            return ADDRESS_TYPE_NOT_FOR_ME;
        }
    }
    
    uint8_t flags = cmd[1] >> 5;
    *p = cmd[1] & 0x1F; 

    if (flags == 0) {
        // return "Instance[{}]".format(p)
        // We have a maximum of 5 instances (0..4)
        if (*p < 5) {
            return ADDRESS_TYPE_DEVICE_INSTANCE;
        }
    } else if (flags == 4) {
        // return "InstanceGroup[{}]".format(p)
        // return ADDRESS_TYPE_INSTANCE_GROUP;
    } else if (flags == 6) {
        // return "InstanceType[{}]".format(p)
        if (*p == 1) {
            return ADDRESS_TYPE_INSTANCE_TYPE;
        }
    } else if (flags == 1) {
        // return "FeatureInstanceNumber[{}]".format(p)
        // return ADDRESS_TYPE_FEATURE_INSTANCE_NUMBER;
    } else if (flags == 5) {
        // return "FeatureInstanceGroup[{}]".format(p)
        // return ADDRESS_TYPE_FEATURE_INSTANCE_GROUP;
    } else if (flags == 3) {
        // return "FeatureInstanceType[{}]".format(p)
        // if (*p == 1) {
        //     return ADDRESS_TYPE_FEATURE_INSTANCE_TYPE;
        // }
    } else if (cmd[1] == 0xFD) {
        // return "FeatureInstanceBroadcast"
        return ADDRESS_TYPE_FEATURE_INSTANCE_BROADCAST;
    } else if (cmd[1] == 0xFF) {
        // return "InstanceBroadcast"
        return ADDRESS_TYPE_INSTANCE_BROADCAST;
    } else if (cmd[1] == 0xFC) {
        // return "FeatureDevice"
        return ADDRESS_TYPE_FEATURE_DEVICE;
    } else if (cmd[1] == 0xFE) {
        // for the device itself.
        return ADDRESS_TYPE_DEVICE;
    }
    return ADDRESS_TYPE_NOT_FOR_ME;
}

static response_type_t processCommand(receive_event_received_t *evt, uint8_t *response) {
    if (evt->numBits != 24) {
        // By default, ignore all commands
        if (evt->numBits != 16) {
            log_uint8("Invalid numBits", evt->numBits);
        }
        return RESPONSE_IGNORE;
    } 

    uint8_t cmd[3];
    cmd[0] = evt->data >> 16;
    cmd[1] = evt->data >> 8;
    cmd[2] = evt->data;
    uint8_t instance;

    /*
    Device Events are specified as 
Short Address Event + Instance Type	b0AAAAAA0	b0TTTTTDD	bDDDDDDDD				b23 ==0 && b15 == 0
Short Address + Instance_number Event	b0AAAAAA0	b1IIIIIDD	bDDDDDDDD				b23 ==0 && b15 == 1
Device Group Event	b10GGGGG0	b0TTTTTDD	bDDDDDDDD				b23 ==1, b22 = 0 && b15 == 0
Instance Group Event	b11GGGGG0	b0TTTTTDD	bDDDDDDDD				b23 ==1, b22 = 1 && b15 == 0
Type + Instance Number event	b10TTTTT0	b1IIIIIDD	bDDDDDDDD				b23 ==1, b22 = 0 && b15 == 1

    Pushbutton is Instance Type 1
    */


    currentCommand.addressing = getAddressType(cmd, &instance);

    // If we're quiescent don't respond to anything other than a stop quiescent.
    if (currentCommand.addressing == ADDRESS_TYPE_NOT_FOR_ME || (config.mode == DEVMODE_QUIESCENT && !(cmd[1] == 0xFE && cmd[2] == CMD_StopQuiescentMode))) {
        return RESPONSE_IGNORE;
    }


    switch (currentCommand.addressing) {
            
        case ADDRESS_TYPE_SPECIAL_DEVICE_COMMAND:
            return processSpecialDeviceCommand(cmd[1], cmd[2], 0, response);

        case ADDRESS_TYPE_SPECIAL_DEVICE_COMMAND_TWOARGS:
            return processSpecialDeviceCommand(cmd[0], cmd[1], cmd[2], response);

        case ADDRESS_TYPE_NOT_FOR_ME:
        case ADDRESS_TYPE_EVENT:
            return RESPONSE_IGNORE;

        default:
            return processDeviceCommand(cmd[2], response);
    }
    // Default response is to ignore, but we should never get here.
    return RESPONSE_IGNORE;
}

static void transmit_response_completed(transmit_event_t event_type) {
    // 3. at least 6 half bit periods (2.4ms) before processing a new command
    notifyCallCompletion();
    startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_POST_RESPONSE_DELAY_MSEC), transmitNextCommandOrWaitForRead);
}

static void transmit_response() {
    clearTimeout();

    transmit(currentCommand.response.mine.val, 8, transmit_response_completed);
}

static void responseFromOtherDeviceReceived(receive_event_t *evt) {
    memcpy(&currentCommand.response.other, evt, sizeof(receive_event_t));
    notifyCallCompletion();
    transmitNextCommandOrWaitForRead();
}



void setCommandObserver(user_mode_callback_t cb) {
    commandObserver = cb;
}


static void commandReceived(receive_event_t *evt) {
    bool skipProcessing = false;

    switch (evt->type) {
        case RECEIVE_EVT_RECEIVED:
            if (evt->rcv.numBits == 25) {
                // there appears to be some weird bug where the Tridonic controller is injecting an extra 1 between the
                // 2nd and 3rd bytes, but only if the last bit of the second byte is a 1.
                evt->rcv.data = ((evt->rcv.data >> 1) & 0xFFFF00) | (evt->rcv.data & 0xFF);
                evt->rcv.numBits--;
            }


            // Check to see if we're expecting a repeat.  at this point currentCommand is actually the last executed command.
            currentCommand.repeatCount = 1;
            if (currentCommand.outcome == RESPONSE_REPEAT) {
                // Last command required a repeat - Check to see if we've received exactly the same thing again.
                if (currentCommand.val == evt->rcv.data && currentCommand.len == evt->rcv.numBits) {
                    currentCommand.repeatCount = 2;
                } else {
                    // Nope... Ignore it. 
                    skipProcessing = true;
                }
            } 

            currentCommand.addressing = ADDRESS_TYPE_NOT_FOR_ME;
            currentCommand.val = evt->rcv.data;
            currentCommand.len = evt->rcv.numBits;
            currentCommand.outcome = skipProcessing ? RESPONSE_IGNORE : processCommand(&(evt->rcv), &currentCommand.response.mine.val);
            
            switch (currentCommand.outcome) {
                case RESPONSE_RESPOND:
                    // 1. Wait 5.5 ms (the minimum delay)
                    // 2. Then send the response (1 byte)
                    startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_DELAY_MSEC), transmit_response);
                    break;
                case RESPONSE_NACK:
                    // Wait for maximum response time.  Ignore any inputs during this time.  
                    notifyCallCompletion();
                    startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), transmitNextCommandOrWaitForRead);
                    break;
                case RESPONSE_REPEAT:
                    // Wait for up to maximum period (this is only 19ms, not the 100 it should be, but it'll work)
                    notifyCallCompletion();
                    waitForRead(MSEC_TO_TICKS(65535), commandReceived);
                    break;

                case RESPONSE_INVALID_INPUT:
                    // This isn't a valid outcome from processCommand.
                case RESPONSE_IGNORE:
                    // Potentially read in a response from the other device.
                    waitForRead(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), responseFromOtherDeviceReceived);
                    break;
            }
            break;
        case RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD:
            // This only ever happens in the REPEAT state when nothing is received during the timeout.
            currentCommand.outcome = RESPONSE_INVALID_INPUT; // This disables the repeat mechanism.
            wait_for_command();
            break;
        case RECEIVE_EVT_INVALID_SEQUENCE:
            currentCommand.outcome = RESPONSE_INVALID_INPUT;
            currentCommand.val = 0;
            currentCommand.len = 0;
            startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), transmitNextCommandOrWaitForRead);
            break;
    }
}


void wait_for_command() {
    waitForRead(0, commandReceived);
}

