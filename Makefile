DEVICE     = attiny804
CLOCK      = 3333333
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
	$(COMPILE) -c main.c -o main.o
	$(COMPILE) -c buttons.c -o buttons.o
	$(COMPILE) -c dali_read.c -o dali_read.o
	$(COMPILE) -c dali_write.c -o dali_write.o
	$(COMPILE) -c queue.c -o queue.o
	$(COMPILE) -c console.c -o console.o
	$(COMPILE) -o $(FILENAME).elf main.o buttons.o dali_read.o dali_write.o queue.o console.o
	avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex $(FILENAME).elf $(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

erase:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) erase


upload:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f $(FILENAME).hex --verify

clean:
	rm -f main.o buttons.o dali_read.o dali_write.o queue.o console.o
	rm -f main.elf
	rm -f main.hex
