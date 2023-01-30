#!/usr/bin/env python3
import logging
logging.basicConfig(format="%(levelname)s: %(message)s", level=logging.WARNING)
import pymcuprog
from pymcuprog.backend import SessionConfig
from pymcuprog.toolconnection import ToolSerialConnection
from pymcuprog.backend import Backend
from pymcuprog.deviceinfo.memorynames import MemoryNames
from struct import pack, unpack
from binascii import hexlify


clock_divisor = 2
clock_freq = 32768

def ms_to_ticks(ms):
    return int(ms * clock_freq / 1000 / clock_divisor)

def ticks_to_ms(ticks):
    return (ticks / clock_freq * 1000 * clock_divisor)


sessionconfig = SessionConfig("attiny804")

# Instantiate USB transport (only 1 tool connected)
transport = ToolSerialConnection("/dev/ttyUSB0")

backend = Backend()
backend.connect_to_tool(transport)
backend.start_session(sessionconfig)


config = bytes(backend.read_memory(MemoryNames.USER_ROW, 0, 12)[0].data)


(num_buttons, target1, target2, target3, target4, target5, shortTime, longTime, repeatTime) = unpack("<BBBBBBHHH", config)
targets = [target1, target2, target3, target4, target5]
print("targets", targets[0:num_buttons])
print("Short time is {:04x} ({} ms)".format(shortTime, ticks_to_ms(shortTime)))
print("Long press time is {:04x} ({} ms)".format(longTime, ticks_to_ms(longTime)))
print("Repeat time is  {:04x} ({} ms)".format(repeatTime, ticks_to_ms(repeatTime)))