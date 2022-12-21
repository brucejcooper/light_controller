#include "rcv.h"
#include <stdint.h>
#include <stdbool.h>
#include "state_machine.h"
#include "console.h"
#include "intr.h"
#include "timing.h"
#include "config.h"
#include <string.h>

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
    RESPONSE_REPEAT,
} response_type_t;


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




// typedef enum {
//     DIRECT_POWER_TO_ME,
//     ADDRESSED_TO_ME,
//     NOT_ADDRESSED_TO_ME,
//     SPECIAL_COMMAND,
//     BROADCAST_ADDRESSED,
//     BROADCAST
// } address_type_t;



// address_type_t getAddressType(uint8_t addrByte) {
//     if (addrByte && 0xFE == 0xFC) {
//         return BROADCAST_ADDRESSED;
//     }
//     if (addrByte && 0xFE == 0xFE) {
//         return BROADCAST;
//     }
//     if (addrByte > 0xCB) {
//         return NOT_ADDRESSED_TO_ME;
//     }
//     if (addrByte >= 0xA0) {
//         return SPECIAL_COMMAND;
//     }

//     bool addressedToGroup = addrByte & 0x80;
//     bool directCommand = addrByte & 0x01;
//     addrByte >>= 1;
//     if (addressedToGroup) {
//         addrByte &= 0x0F;

//         // userData->groups 
//     } else {
//         // Its an address
//         addrByte &= 0x3F;
//     }
// }

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


static response_type_t processCommand(receive_event_received_t *evt, uint8_t *response) {
    if (evt->numBits != 24) {
        log_uint24("IGN", evt->data);
        // By default, ignore all commands
        return RESPONSE_IGNORE;
    } 


    /*
    Device Events are specified as 
Short Address Event + Instance Type	b0AAAAAA0	b0TTTTTDD	bDDDDDDDD				b23 ==0 && b15 == 0
Short Address + Instance_number Event	b0AAAAAA0	b1IIIIIDD	bDDDDDDDD				b23 ==0 && b15 == 1
Device Group Event	b10GGGGG0	b0TTTTTDD	bDDDDDDDD				b23 ==1, b22 = 0 && b15 == 0
Instance Group Event	b11GGGGG0	b0TTTTTDD	bDDDDDDDD				b23 ==1, b22 = 1 && b15 == 0
Type + Instance Number event	b10TTTTT0	b1IIIIIDD	bDDDDDDDD				b23 ==1, b22 = 0 && b15 == 1

    Pushbutton is Instance Type 1
    */

    uint8_t cmd[3];
    memcpy(cmd, ((uint8_t *) (&evt->data))+1, 3);
    uint8_t deviceId = 0;


    if (cmd[0] & 0x01) {
        // Its a command
        uint8_t discrim = cmd[0] >> 6;
        switch (discrim) {
            case 0:
                // Device command
                deviceId = (cmd[0] >> 1) & 0x1F;

                if (cmd[1] == 0xFE) {
                    // Standard Device command
                } else  {
                    // Device Instance Command
                }
                break;
            case 1:
                break;
            case 2:
                break;
            case 3: 
                // Special Device Command (Broadcasts)
                if (cmd[0] == 0xC1) {
                    return processSpecialDeviceCommand(cmd[1], cmd[2], 0, response);
                } else {
                    switch (cmd[0]) {
                        case CMD_DirectWriteMemory:
                        case CMD_DTR1DTR0:
                        case CMD_DTR2DTR1:
                            return processSpecialDeviceCommand(cmd[0], cmd[1], cmd[2], response);
                            break;
                    }
                }
                break;
        }
    } else {
        // its an event.
    }
    if ((cmd[0] && 0xC1 == 0x01) && cmd[1] == 0xFE) {
        // Its a device command.
        switch (cmd[2]) {
            case CMD_IdentifyDevice:
            break;
        }
    }
    
    // We only respond to 24 bit commands, as we are a controller.
    log_uint24("Ignoring", evt->data);
    log_uint8("Bits", evt->numBits);
    return RESPONSE_IGNORE;
}

static void transmit_response_completed() {
    // 3. at least 6 half bit periods (2.4ms) before processing a new command
    startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_POST_RESPONSE_DELAY_MSEC), transmitNextCommandOrWaitForRead);
}

static void transmit_response() {
    log_uint8("Responding", response);
    transmit(response, 8, transmit_response_completed);
}

static void responseFromOtherDeviceReceived(receive_event_t *evt) {
   switch (evt->type) {
        case RECEIVE_EVT_RECEIVED:
            if (evt->rcv.numBits == 8) {
                log_uint8("OTH", (uint8_t) evt->rcv.data);
            } else {
                log_uint8("received illegal response length of", evt->rcv.numBits);
            }
            break;
        case RECEIVE_EVT_INVALID_SEQUENCE:
            log_info("Invalid DALI sequence received while waiting for response");
            break;
        // Ignore other responses
    }
    transmitNextCommandOrWaitForRead();
}

static void commandReceived(receive_event_t *evt) {
    log_char('!');
    response_type_t outcome;
    switch (evt->type) {
        case RECEIVE_EVT_RECEIVED:
            outcome = processCommand(&(evt->rcv), &response);
            switch (outcome) {
                case RESPONSE_RESPOND:
                    // 1. Wait 5.5 ms (the minimum delay)
                    // 2. Then send the response (1 byte)
                    startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_DELAY_MSEC), transmit_response);
                    break;
                case RESPONSE_NACK:
                    // Wait for maximum response time.  Ignore any inputs during this time.  
                    log_info("(not) Responding NACK");
                    startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), transmitNextCommandOrWaitForRead);
                    break;
                case RESPONSE_IGNORE:
                    // Potentially read in a response from the other device.
                    // log_info("Consuming other device response");
                    waitForRead(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC-5), responseFromOtherDeviceReceived);
                    break;
            }
            break;
        case RECEIVE_EVT_INVALID_SEQUENCE:
            log_info("Invalid DALI sequence received");
            startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), transmitNextCommandOrWaitForRead);

            break;
        case RECEIVE_EVT_NO_DATA_RECEIVED_IN_PERIOD:
            log_info("Impossible timeout received");
            startSingleShotTimer(MSEC_TO_TICKS(DALI_RESPONSE_MAX_DELAY_MSEC), transmitNextCommandOrWaitForRead);
            break;
    }
}


void wait_for_command() {
    log_char('?');
    waitForRead(0, commandReceived);
}

