DEVICE     = attiny806
CLOCK      = 3333333
PORT	   = /dev/ttyUSB0
FILENAME   = boot

# High nibble == window, low nibble = period - loaded into WDT.CTRLA during reset
FUSE_WDTCFG = 0x00
# 1.8V BOD, 1kHz, Disabled, Disabled during Sleep
FUSE_BODCFG = 0x00
# 20Mhz, calibration accessible
FUSE_OSCCFG = 0x02
# No CRC, LV UPDI enabled, EEPROM erased during chip erase
FUSE_SYSCFG0 = 0xF6
# Start up delay 64ms
FUSE_SYSCFG1 = 0x07
# No App Data section
FUSE_APPEND = 0x00
# Boot flash = 1Kb - App starts immediately after
FUSE_BOOTEND = 0x08

# the extra linker command is to relocate the application
COMPILE    = avr-gcc -Wall -Os -DF_CPU=$(CLOCK) -mmcu=$(DEVICE) -nostartfiles -Wl,--section-start=.application=0x800
AVR_GCC_DIR = ../avr
SOURCES    = $(wildcard src/*.c)
OBJECTS    = $(subst src/,build/,$(subst .c,.o,$(SOURCES)))
export PATH := $(shell pwd)/$(AVR_GCC_DIR)/bin:$(PATH)


all: flash

download_gcc:
	wget http://downloads.arduino.cc/tools/avr-gcc-7.3.0-atmel3.6.1-arduino7-x86_64-pc-linux-gnu.tar.bz2 -q -O- | bzcat | tar xv

   
build/%.o: src/%.c
	$(COMPILE) -c $< -o $@

prepare: clean
	mkdir -p build
	
build: prepare $(OBJECTS)
	$(COMPILE) -o build/$(FILENAME).elf $(OBJECTS)
	avr-objcopy -R .eeprom -R .fuse -R .lock -R .signature -O ihex build/$(FILENAME).elf build/$(FILENAME).hex
	avr-size --format=avr --mcu=$(DEVICE) build/$(FILENAME).elf

erase:
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) erase -m flash

flash: merge erase
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -f build/merged.hex --verify

flash_fuses:
	avr-objcopy -j .fuse -O ihex build/$(FILENAME).elf build/fuses.hex
	pymcuprog -t uart -u $(PORT) -d $(DEVICE) write -m fuses -l $(FUSE_WDTCFG) $(FUSE_BODCFG) $(FUSE_OSCCFG) 0xFF 0x00 $(FUSE_SYSCFG0) $(FUSE_SYSCFG1) $(FUSE_APPEND) $(FUSE_BOOTEND)

merge: build
	mergehex -m build/$(FILENAME).hex ../blinky/build/main.hex -o build/merged.hex

reset:
	pymcuprog -t uart -u ${PORT} -d $(DEVICE) reset

clean:
	rm -rf build/
	rm -f main.elf
	rm -f main.hex
