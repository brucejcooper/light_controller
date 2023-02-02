#!/usr/bin/env python3
 
import serial
import sys


with serial.Serial("/dev/ttyUSB4", 115200) as ser:
    if len(sys.argv) <= 1:
        print("Need array of pulse widths")
        sys.exit(1)
    pulses = ' '.join(sys.argv[1:])
    print("Args {}".format(pulses))
    ser.write('pulse 13 {}\r\n'.format(pulses).encode('utf-8'))
        
    line = ser.readline()
    
    while line != b'esp32> \r\n':
        print(line)
        line = ser.readline()
