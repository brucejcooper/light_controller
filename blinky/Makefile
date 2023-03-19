DEVICE     = attiny804
CLOCK      = 3333333
FILENAME   = main
PORT	   = /dev/ttyUSB0
# When compiling for bootloader,  target application uses -Wl,--section-start=.text=0x400 to offset code by 1024 bytes
COMPILE    = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -Wl,--section-start=.text=0x400
AVR_GCC_DIR = ../avr
SOURCES    = $(wildcard src/*.c)
OBJECTS    = $(subst src/,build/,$(subst .c,.o,$(SOURCES)))
export PATH := $(shell pwd)/$(AVR_GCC_DIR)/bin:$(PATH)

all: flash

download_gcc:
	wget http://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2 -q -O- | bzcat | tar xv

   
build/%.o: src/%.c
	$(COMPILE) -c $< -o $@

prepare:
	mkdir -p build
	
build: prepare $(OBJECTS)
	$(COMPILE) -o build/$(FILENAME).elf $(OBJECTS)
	avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex build/$(FILENAME).elf build/$(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) build/$(FILENAME).elf

erase:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) erase

flash: clean build erase
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f ../bootloader/build/boot.hex --verify
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f build/$(FILENAME).hex --verify

clean:
	rm -rf build/
	rm -f main.elf
	rm -f main.hex

reset:
	pymcuprog -t uart -u ${PORT} -d $(DEVICE) reset
