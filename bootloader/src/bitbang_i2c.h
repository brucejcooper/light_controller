#ifndef _bitbang_i2c_h_
#define _bitbang_i2c_h_

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>

#define TWI_SCL_FREQ 100000

#define TWI_BAUD ((((float) F_CPU / (float) TWI_SCL_FREQ) - 10)/2)


typedef enum {
    TWI_INIT = 0, 
    TWI_READY,
    TWI_ERROR
} TWI_Status;

typedef enum{
    TWI_WRITE = 0,
    TWI_READ = 1
} TWI_Direction;

void TWI_init(void);
bool TWI_sendAndReadBytes(uint8_t addr, uint16_t regAddress, uint8_t* data, uint8_t len);

#endif
