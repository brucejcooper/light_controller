#include "config.h"
#include <avr/eeprom.h>
#include <stdlib.h>
#include "console.h"


config_t config = {
    .shortAddr = 0x40, // This is another impossible value, but one that isn't the same as a non-initialised flash
    .numButtons = 5,
    .targets = { MAKE_DALIADDR(0), MAKE_DALIADDR(1), MAKE_DALIADDR(2), MAKE_DALIADDR(3), MAKE_DALIADDR(4) }, 
    .shortPressTimer = 500/20,  // In incfrements of 20 msec.
    .doublePressTimer = 200/20,
    .repeatTimer = 160/20,
    .stuckTimer = 20, // In seconds

    .mode = DEVMODE_NORMAL,
    .randomAddr = { 0, 0, 0},
    .searchAddr = {0xFF, 0xFF, 0xFF},
    .dtr = {.value = 0},
    .dataPtr = NULL,
};


void persistConfig() {
    // eeprom_write_block(&config, &USERROW, PERSISTENT_CONFIG_SIZE);
}


void retrieveConfig() {
    if (USERROW.USERROW0 != 0xFF) {
        // log_info("Reading EEPROM");
        // eeprom_read_block(&config, &USERROW, PERSISTENT_CONFIG_SIZE);
    } else {
        // log_info("No stored EEPROM");
    }
}

void initialiseRNG() {
    // Change ADC0 to a floating pin;
    // read some values off the ADC,
    // call srand.  At the moment, this is going to cause a lot of clashes :P
    // srand(1);
}

void randomiseSearchAddr() {
    for (uint8_t i = 0; i < 3; i++) {
        // config.searchAddr[i] = rand();
    }
}

int8_t searchAddressCompare() {
    int8_t comp = 0;
    for (uint8_t i = 0; comp == 0 && i < 3; i++) {
        comp = config.searchAddr[i] - config.searchAddr[i];
    }
    return comp;
}