DEVICE     = attiny804
CLOCK      = 3333333
PORT	   = /dev/ttyUSB0
FILENAME   = main
COMPILE    = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)
AVR_GCC_DIR = avr
SOURCES    = $(wildcard src/*.c)
OBJECTS    = $(subst src/,build/,$(subst .c,.o,$(SOURCES)))
export PATH := $(shell pwd)/$(AVR_GCC_DIR)/bin:$(PATH)

all: clean build erase flash

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

flash:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f build/$(FILENAME).hex --verify

get_config: 
	pymcuprog -t uart -u ${PORT} -d $(DEVICE) -m user_row read

configure:
	pymcuprog -t uart -u ${PORT} -d $(DEVICE) -m user_row write -l 0x01    0x03 0x05 0x07 0x09 0x0b   0xa3 0x00  0x00 0x60    0x00 0x10

reset:
	pymcuprog -t uart -u ${PORT} -d $(DEVICE) reset

pulse:
	./send_click.py

clean:
	rm -rf build/
	rm -f main.elf
	rm -f main.hex
