#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <stdbool.h>
#include <stdint.h>
#include <avr/eeprom.h>
#include <stdbool.h>

#define DALI_BAUD           (1200)
#define DALI_BIT_USECS      (1000000.0/DALI_BAUD)
#define DALI_HALF_BIT_USECS (DALI_BIT_USECS/2.0)
#define DALI_MARGIN_USECS   (45)
#define USEC_TO_TICKS(u)    ((uint16_t) (((float)u)*(F_CPU/1000000.0) + 0.5))
#define MSEC_TO_TICKS(u)    USEC_TO_TICKS((u)*1000)
#define TICKS_TO_USECS(u)   (uint16_t) ((u)/(F_CPU/1000000.0))

// Reponse delay is 22 half bits, or 9.17 msec
#define DALI_RESPONSE_MAX_DELAY_USEC (22 * DALI_HALF_BIT_USECS)

typedef struct {
    uint8_t numButtons;
    uint8_t targets[5]; // The targets

    uint16_t shortPressTimer; // in ms
    uint16_t doublePressTimer; 
    uint16_t repeatTimer;  
} config_t;


// Our config is stored in the USERROW of EEPROM
#define config ((config_t *) &USERROW)

#endif