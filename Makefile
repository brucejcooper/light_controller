DEVICE     = attiny804
CLOCK      = 20000000
PORT	   = /dev/ttyUSB0
FILENAME   = main
COMPILE    = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
AVR_GCC_DIR = avr
export PATH := $(shell pwd)/$(AVR_GCC_DIR)/bin:$(PATH)

all: usb clean build erase upload

usb:
	ls /dev/ttyUSB*

download_gcc:
	wget http://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2 -q -O- | bzcat | tar xv
    
	
build:
	$(COMPILE) -c $(FILENAME).c -o $(FILENAME).o
	$(COMPILE) -o $(FILENAME).elf $(FILENAME).o
	avr-objcopy -j .text -j .data -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

erase:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) erase


upload:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f $(FILENAME).hex --verify

clean:
	rm -f main.o
	rm -f main.elf
	rm -f main.hex
