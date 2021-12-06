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