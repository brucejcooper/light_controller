

# Downloading AVR compiler for attiny 0 series
The default avr-gcc installed by ubuntu doesn't do the 0 series, so I'm stealing the one that Arduino uses

```bash
wget http://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2 -q -O- | bzcat | tar xv
```

This will create a avr-gcc toolchain in the directory `avr` - The Makefile is set up to use this by default.  You can also just run `make download_gcc` to get it.  I will make this query the arduino repository for the latest version in a later update.


This repository is an experiment I am conducting on how best to do a circuit implemented a million times before.  a light switch dimmer (trailing edge). The idea here is to make something that is both efficient and cheap to build.

[Shelly](https://shelly.cloud) produces good, cheap devices, but I'm worried about their vampiric current draw.

# Theory of operation

1. Devices must operate independently.  If one of these breaks, I don't want the whole house to go dark.
1. Power consumption must be minimised, especially when the light is off.
1. I must be able to control lights via a home automation system (home assistant, or equivalent)
1. It should be as cheap as possible to produce.

So the idea is that none of these devices will have WiFi or ethernet themselves.  That would consume too much current.  Instead, they'll use One-wire bus as a communications mechanism, talking back to a central system (Maybe an ESP32, maybe a Raspberry PI) that will bridge all the data back to the main control node. 


The eventual version will use slightly different components.  For the now I am experimenting with things I have to hand.

The need for an effective ZCD mechanism has driven me down a traditional transformer + LDO approach.  This may seem weird, but from what I've read SMPS are not very good at very low load, and light switches are off more than they are on.  What we need here is something that draws micro-amps when it is switched off.