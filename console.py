#!/usr/bin/python3

import serial
import re
from dali.command import Command
from dali.frame import ForwardFrame
import dali.gear
import dali.device


deviceCommands = [None]*256;
gearCommands = [None]*256;
specialGearCommands = [None]*256;


deviceCommands[0x00] = 'IdentifyDevice'
deviceCommands[0x01] = 'ResetPowerCycleSeen'
deviceCommands[0x10] = 'Reset'
deviceCommands[0x11] = 'ResetMemoryBank'
deviceCommands[0x14] = 'SetShortAddress'
deviceCommands[0x15] = 'EnableWriteMemory'
deviceCommands[0x16] = 'EnableApplicationController'
deviceCommands[0x17] = 'DisableApplicationController'
deviceCommands[0x18] = 'SetOperatingMode'
deviceCommands[0x19] = 'AddToDeviceGroupsZeroToFifteen'
deviceCommands[0x1a] = 'AddToDeviceGroupsSixteenToThirtyOne'
deviceCommands[0x1b] = 'RemoveFromDeviceGroupsZeroToFifteen'
deviceCommands[0x1c] = 'RemoveFromDeviceGroupsSixteenToThirtyOne'
deviceCommands[0x1d] = 'StartQuiescentMode'
deviceCommands[0x1e] = 'StopQuiescentMode'
deviceCommands[0x1f] = 'EnablePowerCycleNotification'
deviceCommands[0x20] = 'DisablePowerCycleNotification'
deviceCommands[0x21] = 'SavePersistentVariables'
deviceCommands[0x30] = 'QueryDeviceStatusResponse'
deviceCommands[0x31] = 'QueryApplicationControllerError'
deviceCommands[0x32] = 'QueryInputDeviceError'
deviceCommands[0x33] = 'QueryMissingShortAddress'
deviceCommands[0x34] = 'QueryVersionNumber'
deviceCommands[0x35] = 'QueryNumberOfInstances'
deviceCommands[0x36] = 'QueryContentDTR0'
deviceCommands[0x37] = 'QueryContentDTR0'
deviceCommands[0x38] = 'QueryContentDTR0'
deviceCommands[0x39] = 'QueryRandomAddressH'
deviceCommands[0x3a] = 'QueryRandomAddressM'
deviceCommands[0x3b] = 'QueryRandomAddressL'
deviceCommands[0x3c] = 'ReadMemoryLocation'
deviceCommands[0x3d] = 'QueryApplicationControlEnabled'
deviceCommands[0x3e] = 'QueryOperatingMode'
deviceCommands[0x3f] = 'QueryManufacturerSpecificMode'
deviceCommands[0x40] = 'QueryQuiescentMode'
deviceCommands[0x41] = 'QueryDeviceGroupsZeroToSeven'
deviceCommands[0x42] = 'QueryDeviceGroupsEightToFifteen'
deviceCommands[0x43] = 'QueryDeviceGroupsSixteenToTwentyThree'
deviceCommands[0x44] = 'QueryDeviceGroupsTwentyFourToThirtyOne'
deviceCommands[0x45] = 'QueryPowerCycleNotification'
deviceCommands[0x46] = 'QueryDeviceCapabilities'
deviceCommands[0x47] = 'QueryExtendedVersionNumber'
deviceCommands[0x48] = 'QueryResetState'
deviceCommands[0x61] = 'SetEventPriority'  # First Instance command.
deviceCommands[0x62] = 'EnableInstance'
deviceCommands[0x63] = 'DisableInstance'
deviceCommands[0x64] = 'SetPrimaryInstanceGroup'
deviceCommands[0x65] = 'SetInstanceGroup1'
deviceCommands[0x66] = 'SetInstanceGroup2'
deviceCommands[0x67] = 'SetEventScheme'
deviceCommands[0x68] = 'SetEventFilter'
deviceCommands[0x80] = 'QueryInstanceType'
deviceCommands[0x81] = 'QueryResolution'
deviceCommands[0x82] = 'QueryInstanceError'
deviceCommands[0x83] = 'QueryInstanceStatus'
deviceCommands[0x84] = 'QueryEventPriority'
deviceCommands[0x86] = 'QueryInstanceEnabled'
deviceCommands[0x88] = 'QueryPrimaryInstanceGroup'
deviceCommands[0x89] = 'QueryInstanceGroup1'
deviceCommands[0x8a] = 'QueryInstanceGroup2'
deviceCommands[0x8b] = 'QueryEventScheme'
deviceCommands[0x8c] = 'QueryInputValue'
deviceCommands[0x8d] = 'QueryInputValueLatch'
deviceCommands[0x8e] = 'QueryFeatureType'
deviceCommands[0x8f] = 'QueryNextFeatureType'
deviceCommands[0x90] = 'QueryEventFilterZeroToSeven'
deviceCommands[0x91] = 'QueryEventFilterEightToFifteen'
deviceCommands[0x92] = 'QueryEventFilterSixteenToTwentyThree'


gearCommands[0x00] = 'Off'
gearCommands[0x01] = 'Up'
gearCommands[0x02] = 'On'
gearCommands[0x03] = 'StepUp'
gearCommands[0x04] = 'StepDown'
gearCommands[0x05] = 'RecallMaxLevel'
gearCommands[0x06] = 'RecallMinLevel'
gearCommands[0x07] = 'StepDownAndOff'
gearCommands[0x08] = 'OnAndStepUp'
gearCommands[0x09] = 'EnableDAPCSequence'
gearCommands[0x0a] = 'GoToLastActiveLevel'
gearCommands[0x0b] = 'ContinuousUp'
gearCommands[0x0c] = 'ContinuousDown'
for i in range(0,16):
    gearCommands[0x10 +i] = 'GoToScene {}'.format(i)

gearCommands[0x20] = 'Reset'
gearCommands[0x21] = 'StoreActualLevelInDTR0'
gearCommands[0x22] = 'SavePersistentVariables'
gearCommands[0x23] = 'SetOperatingMode'
gearCommands[0x24] = 'ResetMemoryBank'
gearCommands[0x25] = 'IdentifyDevice'
gearCommands[0x2a] = 'SetMaxLevel'
gearCommands[0x2b] = 'SetMinLevel'
gearCommands[0x2c] = 'SetSystemFailureLevel'
gearCommands[0x2d] = 'SetPowerOnLevel'
gearCommands[0x2e] = 'SetFadeTime'
gearCommands[0x2f] = 'SetFadeRate'
gearCommands[0x30] = 'SetExtendedFadeTime'

for i in range(0,16):
    gearCommands[0x40 +i] = 'SetScene {}'.format(i)
for i in range(0,16):
    gearCommands[0x50 +i] = 'RemoveFromScene {}'.format(i)
for i in range(0,16):
    gearCommands[0x60 +i] = 'AddToGroup {}'.format(i)
for i in range(0,16):
    gearCommands[0x70 +i] = 'RemoveFromGroup {}'.format(i)
gearCommands[0x80] = 'SetShortAddress'
gearCommands[0x81] = 'EnableWriteMemory'
gearCommands[0x90] = 'QueryStatus'
gearCommands[0x91] = 'QueryControlGearPresent'
gearCommands[0x92] = 'QueryLampFailure'
gearCommands[0x93] = 'QueryLampPowerOn'
gearCommands[0x94] = 'QueryLimitError'
gearCommands[0x95] = 'QueryResetState'
gearCommands[0x96] = 'QueryMissingShortAddress'
gearCommands[0x97] = 'QueryVersionNumber'
gearCommands[0x98] = 'QueryContentDTR0'
gearCommands[0x99] = 'QueryDeviceType'
gearCommands[0x9a] = 'QueryPhysicalMinimum'
gearCommands[0x9b] = 'QueryPowerFailure'
gearCommands[0x9c] = 'QueryContentDTR1'
gearCommands[0x9d] = 'QueryContentDTR2'
gearCommands[0x9e] = 'QueryOperatingMode'
gearCommands[0x9f] = 'QueryLightSourceType'
gearCommands[0xa0] = 'QueryActualLevel'
gearCommands[0xa1] = 'QueryMaxLevel'
gearCommands[0xa2] = 'QueryMinLevel'
gearCommands[0xa3] = 'QueryPowerOnLevel'
gearCommands[0xa4] = 'QuerySystemFailureLevel'
gearCommands[0xa5] = 'QueryFadeTimeFadeRate'
gearCommands[0xa6] = 'QueryManufacturerSpecificMode'
gearCommands[0xa7] = 'QueryNextDeviceType'
gearCommands[0xa8] = 'QueryExtendedFadeTime'
gearCommands[0xaa] = 'QueryControlGearFailure'
for i in range(0,16):
    gearCommands[0xb0+i] = 'QueryScene {} Level'.format(i)
gearCommands[0xc0] = 'QueryGroupsZeroToSeven'
gearCommands[0xc1] = 'QueryGroupsEightToFifteen'
gearCommands[0xc2] = 'QueryRandomAddressH'
gearCommands[0xc3] = 'QueryRandomAddressM'
gearCommands[0xc4] = 'QueryRandomAddressL'
gearCommands[0xc5] = 'ReadMemoryLocation'


specialGearCommands[0xa1] = 'Terminate'
specialGearCommands[0xa3] = 'DTR0'
specialGearCommands[0xa5] = 'Initialise'
specialGearCommands[0xa7] = 'Randomise'
specialGearCommands[0xa9] = 'Compare'
specialGearCommands[0xab] = 'Withdraw'
specialGearCommands[0xad] = 'Ping'
specialGearCommands[0xb1] = 'SearchaddrH'
specialGearCommands[0xb3] = 'SearchaddrM'
specialGearCommands[0xb5] = 'SearchaddrL'
specialGearCommands[0xb7] = 'ProgramShortAddrss'
specialGearCommands[0xb9] = 'VerifyShortAddress'
specialGearCommands[0xbb] = 'QueryShortAddress'
specialGearCommands[0xc1] = 'EnableDeviceType'
specialGearCommands[0xc3] = 'DTR1'
specialGearCommands[0xc5] = 'DTR2'
specialGearCommands[0xc7] = 'WriteMemoryLocation'
specialGearCommands[0xc9] = 'WriteMemoryLocationNoReply'


# Device Selection
# Bit 16 is set for commands, not for events...
# DeviceGroup - b10XXXXX1   f[16], f[23:22] = 0x02 - group is F[21:17]
# DeviceBroadcast = 0xFF (7E << 1 | 1)
# DeviceBroadcast Unaddressed = 0xFD (7E << 1 | 1)


# instance selection.

# InstanceNumber(_AddressedInstance):             _flags = 0x00 (000) + 5 bits
# InstanceGroup(_AddressedInstance):              _flags = 0x80 (100) + 5 bits
# InstanceType(_AddressedInstance):               _flags = 0xC0 (110) + 5 bits
# FeatureInstanceNumber(_AddressedInstance):      _flags = 0x20 (001) + 5 bits
# FeatureInstanceGroup(_AddressedInstance):       _flags = 0xA0 (101) + 5 bits
# FeatureInstanceType(_AddressedInstance):        _flags = 0x60 (011) + 5 bits
# FeatureInstanceBroadcast(_UnaddressedInstance): _val = 0xFD (flags = 111, instance = 11101)
# InstanceBroadcast(_UnaddressedInstance):        _val = 0xFF (flags = 111, instance = 11111)
# FeatureDevice(_UnaddressedInstance):            _val = 0xFC (flags = 111, instance = 11100)
# Device(_UnaddressedInstance):                   _val = 0xFE (flags = 111, instance = 11110)

# Sooo, 01ff99 is an instance broadcast command 99 for devie 0
# during scanning, it appears to send out e.g 01a580 Device 0 Flag 5 group 5? (FeatureInstanceGroup) QueryInstanceType

def device_from_frame(c1):
    deviceId = c1 >> 1
    if deviceId == 0x7F:
        return "Device Broadcast"
    elif deviceId == 0x7E:
        return "Unaddressed Device Broadcast"
    else:
        return "DEV[{}]".format(deviceId)
    


def instance_from_frame(c2):
    flags = c2 >> 5
    p = c2 & 0x1F; 
    if flags == 0:
        return "Instance[{}]".format(p)
    elif flags == 4:
        return "InstanceGroup[{}]".format(p)
    elif flags == 6:
        return "InstanceType[{}]".format(p)
    elif flags == 1:
        return "FeatureInstanceNumber[{}]".format(p)
    elif flags == 5:
        return "FeatureInstanceGroup[{}]".format(p)
    elif flags == 3:
        return "FeatureInstanceType[{}]".format(p)
    elif c2 == 0xFD:
        return "FeatureInstanceBroadcast"
    elif c2 == 0xFF:
        return "InstanceBroadcast"
    elif c2 == 0xFC:
        return "FeatureDevice"
    elif c2 == 0xFE:
        return "No Instance"
    return "reserved"




def decorate(state, input, addressing, direction, output):
    print("{} {:6s} {} {:1s} {:4s} #".format(state, input, addressing, direction, output), end="")


    if len(input) == 6:
        c1 = int(input[0:2], 16)
        c2 = int(input[2:4], 16)
        c3 = int(input[4:6], 16)

        deviceId = device_from_frame(c1)
        instanceId = instance_from_frame(c2)

        if c1 & 0x01 != 0:
            print("{} {} {}".format(deviceId, instanceId, deviceCommands[c3]))
            return
        else:
            b23 = c1 & 0x80
            b22 = c1 & 0x40
            b15 = c2 & 0x80

            id1 = (c1 >> 1) & 0x3F
            id2 = (c2 >> 1) & 0x3F
            eventBits = ((c2 & 0x03) << 8) | c1

            # Its an event.  
            if b23 == 0:
                group1 = "device"
                if b15 != 0:
                    # short address + instance number
                    group2 = "instance"
                else:
                    group2 = "type"
                    # short address + instance type
            elif b22 == 0 and b15 == 0:
                group1 = "device group"
                group2 = "type"
            elif b22 != 0 and b15 == 0:
                group1 = "instance group"
                group2 = "type"
            else:
                # Type + Instance number  event
                group1 = "Type"
                group2 = "Instance"
            # print("{} {} {} {} {} EVENT {} -  {} {} {} {} {}".format(state, group1, id1, group2, id2, hex(eventBits), hex(c1), hex(c2), hex(c3), direction, output))
            # return
    elif len(input) == 4:
        c1 = int(input[0:2], 16)
        c2 = int(input[2:4], 16)

        if c1 == 0xFF:
            # Broadcast.
            print("Gear Broadcast {}".format(gearCommands[c2]))
            return
        elif (c1 & 0x81) == 0x01:
            # Gear command
            print("Gear[{}] {}".format((c1 >> 1), gearCommands[c2]))
            return
        elif (c1 & 0x81) == 0x81 and c1 > 0b10100000 and c1 <= 0b11001011:
            # Device special command.
            print("Special Gear {}({})".format(specialGearCommands[c1], c2))
            return

    # Fall back to asking python-dali
    parsed = Command.from_frame(ForwardFrame(len(x[2])*4, int(input, 16)))
    print(parsed);


with serial.Serial('/dev/ttyUSB1', baudrate=115200) as ser:
    while True:
        lines = ser.readline().decode('utf8').split('\r')
        for l in lines:
            x = re.match("^(.),([0-9a-z]{4,6}),(.),(.?),(.*)$", l)
            if x:
                decorate(x[1], x[2], x[3], x[4], x[5])
            elif l != '\n' and l != 'xx'  and l != 'x':
                print(l)


