/* 
  sample UART software
  transmit serial data at 9600,N,8,1
  code for avr-gcc
           ATTiny85 at 8 MHz
  code in public domain
*/

#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdio.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit)) // clear bit
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))  // set bit

/* ATTiny85 IO pins
             ___^___
           -|PB5 VCC|-
           -|PB3 PB2|-
Load       -|PB4 PB1|- GND (-ve analog compare)
           -|GND PB0|- Zero cross in
             -------
*/

/* prototypes */
// main routines
void setup(void);
int main(void);

// With Interrupt delays, 76 is the highest value that is not on 100%
volatile uint8_t duty = 77/2;


/*** ISR ***/


ISR(ANA_COMP_vect) {
    sbi(PORTB, PB4); // Turn on load
    sbi(GTCCR, PSR0); // Clear the prescaler
    OCR0A = round(duty); // 79 ticks at /1024 is 10.112 ms, which will never fire, because a new ZC will occur first.
    TCNT0 = 0; // Reset the clock to zero.
}

ISR(TIM0_COMPA_vect) {
	cbi(PORTB, PB4); // Turn off load
}


void setup(void) {
    // Set up Analog Comparator
    cbi(ADCSRB, ACME); // Disable ACME, making AIN1 the -ve end of the comparator.
    ACSR = (1 << ACIE) | (0 << ACIS0) | (0 << ACIS0); // Interrupt on toggle

    // Set up Timer0
    TCCR0A = 0; // normal mode.
    TCNT0 = 0;
    TCCR0B = (1 << CS02) | (1 << CS00); // clock runs in /1024 mode
    OCR0A = 255; // Initially, make it the highest delay - 32 milliseconds - We will receive a ZC before then.
    sbi(TIMSK, OCIE0A);


    sbi(DDRB, PB4); // PB4 is output
    cbi(PORTB, PB4); // and it is zero

    // Turn off peripherals, to save power
    sbi(PRR, PRTIM1);
    sbi(PRR, PRUSI);
    sbi(PRR, PRADC);

    // Enable Sleeping (Default, IDLE mode)
    sbi(MCUCR, SE);
    sleep_enable();
    sei();
}

int main(void) {
  setup();
  for(;;) {
      // Check duty,  
      // if 0% or 100%, set the output appropriately, turn off timers and ZCD, then SLEEP, awaiting Interrupt on Power button or protocol - Pity you can't Power down (two level interrupts)
      // If anything else, turn on ZCD, then SLEEP. 
       sleep_cpu();
  }
};