#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <stdbool.h>
#include <stdint.h>
#include <avr/eeprom.h>

typedef struct {
    uint8_t numButtons;
    uint8_t targets[5]; // The targets

    uint16_t shortPressTimer; // in ms
    uint16_t doublePressTimer; 
    uint16_t repeatTimer;  
} config_t;


#define config ((config_t *) &USERROW)

#endif