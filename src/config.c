#include "config.h"
#include <avr/eeprom.h>
#include <stdlib.h>

config_t config = {
    .shortAddr = 0x40, // This is another impossible value, but one that isn't the same as a non-initialised flash
    .numButtons = 5,
    .targets = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 
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
    eeprom_write_block(&config, &USERROW, PERSISTENT_CONFIG_SIZE);
}


void retrieveConfig() {
    if (eeprom_read_byte((const uint8_t *) &USERROW.USERROW0) != 0xFF) {
        eeprom_read_block(&config, &USERROW, PERSISTENT_CONFIG_SIZE);
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