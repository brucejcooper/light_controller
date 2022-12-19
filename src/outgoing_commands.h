#ifndef __OUTGOING_COMMANDS_H__
#define __OUTGOING_COMMANDS_H__

#include "snd.h"

typedef enum  {
    // Addressed commands
    COMMAND_Off = 0x00, // No response				
    COMMAND_Up = 0x01, // No response				
    COMMAND_On = 0x02, // No response				
    COMMAND_StepUp = 0x03, // No response				
    COMMAND_StepDown = 0x04, // No response				
    COMMAND_RecallMaxLevel = 0x05, // No response				
    COMMAND_RecallMinLevel = 0x06, // No response				
    COMMAND_StepDownAndOff = 0x07, // No response				
    COMMAND_OnAndStepUp = 0x08, // No response				
    COMMAND_EnableDAPCSequence = 0x09, // No response				
    COMMAND_GoToLastActiveLevel = 0x0a, // No response				
    COMMAND_ContinuousUp = 0x0b, // No response				
    COMMAND_ContinuousDown = 0x0c, // No response				
    COMMAND_GoToScene = 0x10, // | bSSSS S = Scene number	N./A				
    COMMAND_Reset = 0x20, // No response	2			
    COMMAND_StoreActualLevelInDTR0 = 0x21, // No response	2	DTR0		
    COMMAND_SavePersistentVariables = 0x22, // No response	2			
    COMMAND_SetOperatingMode = 0x23, // No response	2	DTR0		
    COMMAND_ResetMemoryBank = 0x24, // No response	2	DTR0		
    COMMAND_IdentifyDevice = 0x25, // No response	2			
    COMMAND_SetMaxLevel = 0x2a, // No response	2	DTR0		
    COMMAND_SetMinLevel = 0x2b, // No response	2	DTR0		
    COMMAND_SetSystemFailureLevel = 0x2c, // No response	2	DTR0		
    COMMAND_SetPowerOnLevel = 0x2d, // No response	2	DTR0		
    COMMAND_SetFadeTime = 0x2e, // No response	2	DTR0		
    COMMAND_SetFadeRate = 0x2f, // No response	2	DTR0		
    COMMAND_SetExtendedFadeTime = 0x30, // No response	2	DTR0		
    COMMAND_SetScene = 0x40, //| bSSSS	N./A	2	DTR0		S = Scene number
    COMMAND_RemoveFromScene = 0x50, //| bSSSS	N./A	2			S = Scene number
    COMMAND_AddToGroup = 0x60, //| bGGGG	N./A	2			G = Group ID
    COMMAND_RemoveFromGroup = 0x70, //| bGGGG	N./A	2			G = Group ID
    COMMAND_SetShortAddress = 0x80, //2	DTR0		
    COMMAND_EnableWriteMemory = 0x81, //2			
    COMMAND_QueryStatus = 0x90, // Bitmap	
    COMMAND_QueryControlGearPresent = 0x91, // Yes/No	
    COMMAND_QueryLampFailure = 0x92, // Yes/No	
    COMMAND_QueryLampPowerOn = 0x93, // Yes/No	
    COMMAND_QueryLimitError = 0x94, // Yes/No	
    COMMAND_QueryResetState = 0x95, // Yes/No	
    COMMAND_QueryMissingShortAddress = 0x96, // Yes/No	
    COMMAND_QueryVersionNumber = 0x97, //bSSSSVVVV	0 = original, 2009 = 1 << 4
    COMMAND_QueryContentDTR0 = 0x98, //DTR0	Numeric	
    COMMAND_QueryDeviceType = 0x99, //Numeric	
    COMMAND_QueryPhysicalMinimum = 0x9a, //NumericResponseMask?	
    COMMAND_QueryPowerFailure = 0x9b, // Yes/No	
    COMMAND_QueryContentDTR1 = 0x9c, //DTR1	Numeric	
    COMMAND_QueryContentDTR2 = 0x9d, //DTR2	Numeric	
    COMMAND_QueryOperatingMode = 0x9e, //Numeric	
    COMMAND_QueryLightSourceType = 0x9f, //Numeric
    COMMAND_QueryActualLevel = 0xa0, //NumericResponseMask	
    COMMAND_QueryMaxLevel = 0xa1, //NumericResponseMask	
    COMMAND_QueryMinLevel = 0xa2, //NumericResponseMask	
    COMMAND_QueryPowerOnLevel = 0xa3, //NumericResponseMask	
    COMMAND_QuerySystemFailureLevel = 0xa4, //NumericResponseMask	
    COMMAND_QueryFadeTimeFadeRate = 0xa5, //fade time << 4 | fade rate	
    COMMAND_QueryManufacturerSpecificMode = 0xa6, // Yes/No	
    COMMAND_QueryNextDeviceType = 0xa7, //See QueryDeviceType	
    COMMAND_QueryExtendedFadeTime = 0xa8, //Numeric	
    COMMAND_QueryControlGearFailure = 0xaa, // Yes/No	
    COMMAND_QuerySceneLevel = 0xb0, //| bSSSS				Numeric	
    COMMAND_QueryGroupsZeroToSeven = 0xc0, //Bitfield each bit representing membership of a group	
    COMMAND_QueryGroupsEightToFifteen = 0xc1, //Bitfield each bit representing membership of a group	
    COMMAND_QueryRandomAddressH = 0xc2, //Numeric	
    COMMAND_QueryRandomAddressM = 0xc3, //Numeric	
    COMMAND_QueryRandomAddressL = 0xc4, //Numeric	
    COMMAND_ReadMemoryLocation = 0xc5, //Numeric	


    // Special Commands are broadcast (b10100000 to 11001011,  i.e. Group bit set, and group higher than 16, plus we can use the LSB)
    COMMAND_Terminate=0xa1,             //	0	N/A				A = Addressed	
    COMMAND_DTR0=0xa3,                  //	VAL			DTR0			
    COMMAND_Initialise=0xa5,            //	
    COMMAND_Randomise=0xa7,             //	0		2				
    COMMAND_Compare=0xa9,               //					Y/N		
    COMMAND_Withdraw=0xab,              //							
    COMMAND_Ping = 0xad,                //							Ignored by control gear
    COMMAND_SearchaddrH=0xb1,           //	ADDR						
    COMMAND_SearchaddrM=0xb3,           //	ADDR						
    COMMAND_SearchaddrL=0xb5,           //	ADDR						
    COMMAND_ProgramShortAddrss	= 0xb7, // Matches Search Addr	ADDR << 1 | 1, 0xFF == MASK						Only ballast matching HML 
    COMMAND_VerifyShortAddress	= 0xb9, // Matches Search Addr	ADDR << 1 | 1, 0xFF == MASK				Y/N		
    COMMAND_QueryShortAddress	= 0xbb, // Matches Search Addr					Numeric		Only ballast matching HML 
    COMMAND_EnableDeviceType=0xc1,      //	DEVTYPE						
    COMMAND_DTR1=0xc3,                  //	VAL			DTR1			
    COMMAND_DTR2=0xc5,                  //	VAL			DTR2			
    COMMAND_WriteMemoryLocation=0xc7,   //	VAL			DTR0 = Location, DTR1 = Bank	Numeric (what it was?)		DTR0 gets incremented
    COMMAND_WriteMemoryLocationNoReply=0xc9, //	VAL			DTR0 = Location, DTR1 = Bank			
} outgoing_command_t;

typedef enum  {
    COMMAND_RESPONSE_NACK,
    COMMAND_RESPONSE_VALUE,
    COMMAND_RESPONSE_ERROR,
} outgoing_command_response_type_t;


typedef void (*response_handler_t)(outgoing_command_response_type_t responseType, uint8_t response);
void transmitCommand(uint16_t val, response_handler_t handler);

#endif