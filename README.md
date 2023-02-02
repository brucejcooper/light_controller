# Light Switch firmware
This repo consists of the code that I'm using for my own home automation light switches.


### Theory of operation

1. Devices must operate independently.  If one of these breaks, I don't want the whole house to go dark.
1. Power consumption must be minimised, especially when the light is off.
1. I must be able to control lights via a home automation system (home assistant, or equivalent)
1. It should be as cheap as possible to produce.

So the idea is that none of these devices will have WiFi or ethernet themselves.  That would consume too much current.  Instead, We will use the DALI protocol, which is baked into the transformers that I'll be using for my home.  The circuit for driving a dali signal is very simple, with the exception of isolated power (needed to make the device safe in the presence of a 220V AC wire).  All isolated power systems that I'm aware of use a large amount of quiescent current (on the order of 10s of mA).  This would overload the DALI power supply. 

# Tech notes

## Downloading AVR compiler for attiny 0 series
The default avr-gcc installed by ubuntu doesn't do the 0 series, so I'm stealing the one that Arduino uses

```bash
wget http://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2 -q -O- | bzcat | tar xv
```

This will create a avr-gcc toolchain in the directory `avr` - The Makefile is set up to use this by default.  You can also just run `make download_gcc` to get it.  I will make this query the arduino repository for the latest version in a later update.


This repository is an experiment I am conducting on how best to do a circuit implemented a million times before.  a light switch dimmer (trailing edge). The idea here is to make something that is both efficient and cheap to build.

[Shelly](https://shelly.cloud) produces good, cheap devices, but I'm worried about their vampiric current draw.



# On Matter
I experimented with matter, and got it working quite well.  I have a couple of problems however
1. Its pretty power hungry to run a radio transmitter.  It will use 10s of milliamps for short periods, fairly often.  We might be able to get away with a large capacitor, or we might be able to set it up as a SED so that they wake up less often. Or we might run a second power cble to supply more power.  Seems like overkill, however
2. Transactions can take some time.  Each time a session is set up, a new encryption channel is set up.  This takes time.  I was seeing hundreds of milliseconds of latency, which becomes noticeable.  Now a few hundred milliseconds for a voice command is fine, but when I push a button, I want the light to react instantly.
3. NRF chipsets cost more, and require more external parts



# Keeping it simple
So here's what I've landed on. I want the switches themselves to be as simple as possible, and to use as little power as possible.  It should be sleeping 99.9999% of the time. It shouldn't wake up unless its got something to do (i.e. if something else transmits that is no reason for the rest of the devices to wake up and listen)

In order to facilitate this, configuration will be done via an OOB mechanism, rather than via the DALI protocol, as this will mean that the devices don't need to listen all the time. 

1. The device will sleep all the time
1. Wake up source is NFC, UPDI, or a button press.
1. Do what you need to do, go back to sleep once the button is released.

I have discovered that taking an event driven coding mechanism wastes flash.  I've now simplified it to just do things sequentially in the main loop, and deal with the fact that transmit and receive can take milliseconds.  Button timings will be much longer than this anway, so we can just take a quick break.  In some circumstances, this happens during a debounce period anyway.

The consequence of this is that all configuration needs to be done locally. The idea is to attach a NFC loop that will listen, wake up the chip, and boot into a bootloader - The use of a bootloader allows us to flash over NFC as well.   NFC will also power the chip (temporarily) allowing the device to be configured before being connected to DALI bus.  (But how, we want NFC to work even after DALI is connected.  The chip will put out 2V, so a diode would cause an unacceptable voltage drop.  Do I need a Power Mux e.g TPS2116.  I probably do.  Luckily they are pretty cheap. 

A simpler way to do it is to just use UPDI - but this means adding some sort of connector to the case and also has the same multi-supply problem (which could be fixed with a diode or a switch).


## Bootloader

Needs a mechanism to automatically reboot the MCU when the NFC is being used - We could rely upon the main firmware to detect the field then reboot itself, but what if it crashes?  Maybe the WDT?

Each FD transition from high -> low (SRAM filled up) could be turned into a quick pulse on RESET (minimum requirement of 2.5us), using a monostable multivibrator (SN74LVC1G123).  The bootloader now boots each time a FD goes low, but we might need multiple transactions to flash the device. SRAM can only store 64 bytes at a time.  We don't want to boot into a half written flash, so perhaps the Bootloader needs to check an EEPROM value.  

If there were two slots, we could check if we're complete or not.  We'd repeatedly reboot into the old firmware until the last block was written. 

## WDT
Do I need a WDT?  Its probably a good idea.



# Pins
* 2 Voltage supply
* 1 Reset/UPDI
* 5 switches
* 2 DALI TX/RX
* 4 left over. Not quite enough for LEDs. 

GPIO expander for LEDs? Or just go to 20 pin package (which would also get us I2C). Attiny406 is $1.26.  Attiny404 is $1.05 so $0.21 difference. Much cheaper than a separate chip. 
