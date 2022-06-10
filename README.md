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

