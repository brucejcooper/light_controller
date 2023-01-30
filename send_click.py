#!/usr/bin/env python3
 
import serial


with serial.Serial("/dev/ttyUSB4", 115200) as ser:
    ser.write(b'pulse 13 200\r\n')
        
    line = ser.readline()
    
    while line != b'esp32> \r\n':
        print(line)
        line = ser.readline()
