DEVICE     = attiny804
CLOCK      = 3333333
PORT	   = /dev/ttyUSB0
FILENAME   = main
COMPILE    = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
AVR_GCC_DIR = avr
OBJECTS    = main.o \
             console.o \
			 intr.o \
			 buttons.o \
			 rcv.o \
			 snd.o \
			 incoming_commands.o \
			 outgoing_commands.o \
			 idle.o
export PATH := $(shell pwd)/$(AVR_GCC_DIR)/bin:$(PATH)

all: usb clean build erase upload

usb:
	ls /dev/ttyUSB*

download_gcc:
	wget http://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2 -q -O- | bzcat | tar xv

   
.c.o:
	$(COMPILE) -c $< -o $@

	
build: $(OBJECTS)
	$(COMPILE) -o $(FILENAME).elf $(OBJECTS)
	avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

erase:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) erase


upload:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f $(FILENAME).hex --verify

clean:
	rm -f $(OBJECTS)
	rm -f main.elf
	rm -f main.hex
