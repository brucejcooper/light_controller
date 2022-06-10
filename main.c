/* 
  DALI master, useable as a multi-switch controller.  Programmed (via UPDI, or UART?) to respond to button presses
  by sending out DALI commands to dim 
*/

#define __DELAY_BACKWARD_COMPATIBLE__

#include <avr/io.h>
#include <util/delay.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdio.h>
#include <util/atomic.h>

/*
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit)) // clear bit
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))  // set bit

/ * attiny804 IO pins
             ___^___
           -|PB5 VCC|-
           -|PB3 PB2|-
Load       -|PB4 PB1|- GND (-ve analog compare)
           -|GND PB0|- Zero cross in
             -------
* /

/ * prototypes * /
// main routines
void setup(void);
int main(void);

// With Interrupt delays, 76 is the highest value that is not on 100%
volatile uint8_t duty = 77/2;


/ *** ISR *** /


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
*/

#define MIN(a,b) (a < b ? a : b)

#define STEP_DELAY          10
#define THRESHOLD           100					/* 100 steps x 10 ms/step -> 1000 ms */
#define LONG_DELAY          500
#define SHORT_DELAY         100
#define NUMBER_OF_BLINKS    3

// Switch is on PB2
#define SWITCH_PORT PORTB
#define SWITCH_bm   PIN2_bm

// LED is on PA4
#define LED_PORT PORTA
#define LED_bm   PIN4_bm

volatile uint8_t pb2Ioc;



/*
 * Events that can occur 
 * 1. a button short press (x 5 buttons) - GPIO interrupt, plus timers for debounce (too short a press should be ignored as a debounce)
 * 2. a button long press (x 5 buttons) - as above, but the timer is ongoing. 
 * 3. There is a DALI command to send (in the queue) and the bus is idle.
 * 4. a DALI receive starts - GPIO change interrupt plus timing (on the order of milliseconds).
 * 
 */

#define NUM_BUTTONS (5)

typedef enum {
    BTN_RELEASED,
    BTN_DEBOUNCE,
    BTN_SHORT_PRESS,
    BTN_LONG_PRESS,
} switchstate_t;

switchstate_t button_states[NUM_BUTTONS] = { BTN_RELEASED, BTN_RELEASED, BTN_RELEASED, BTN_RELEASED, BTN_RELEASED };
uint16_t next_button_timeout[NUM_BUTTONS];

// We don't assume that the PORTB pin numbers line up exactly with the switches (maybe we can in the future).  this maps it
const uint16_t button_masks[NUM_BUTTONS] = {
    PIN1_bm,
    PIN2_bm,
    PIN3_bm,
    PIN4_bm,
    PIN5_bm,
};

// Messing with the LED.
//            LED_PORT.OUTSET = LED_bm;
//            LED_PORT.OUTCLR = LED_bm;


ISR(PORTB_PORT_vect)
{
    for (int pin = 0; pin < NUM_BUTTONS; pin++) {
        uint8_t bm = button_masks[pin];
        switchstate_t swState = button_states[pin];

        if(SWITCH_PORT.INTFLAGS & bm) {
        // Clears the interrupt flag.
        SWITCH_PORT.INTFLAGS &= bm;

        if (SWITCH_PORT.IN & bm) {
            // Released.
            // Turn off any button related timers.
            if (swState == BTN_SHORT_PRESS) {
                // It was released after the debounce period, but before it turned into a long press.  Issue the short press command.
            }
            button_states[pin] = BTN_RELEASED;
        } else {
            // High to Low. 
            button_states[pin] = BTN_DEBOUNCE;
            // Set a timer for the debounce period. 
        }
    }
}



void long_delay(int16_t ms) {
  while (ms > 0) {
    int16_t amt = MIN(10, ms);
    _delay_ms(amt);
    ms -= amt;
  }
}

void LED_blink(uint32_t time_ms)
{
    for (uint8_t i = 0; i < NUMBER_OF_BLINKS; i++)
    {
        LED_PORT.OUT |= LED_bm;
        long_delay(time_ms);
        LED_PORT.OUT &= ~LED_bm;
        long_delay(time_ms);
    }
}




int main(void)
{
    uint8_t counter = 0;

    SWITCH_PORT.DIRCLR = SWITCH_bm; /* set PB2 as input */
    SWITCH_PORT.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc; /* enable internal pull-up */
    LED_PORT.DIRSET = LED_bm; /* set PB5 as output */

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sei();

    while (1)
    {
        sleep_mode();
        // if (~SWITCH_PORT.IN & SWITCH_bm) /* check if PB2 is pulled to GND */
        // {
        //     while (~SWITCH_PORT.IN & SWITCH_bm) /* wait until PB2 is pulled to VDD */
        //     {
        //         _delay_ms(STEP_DELAY);
        //         counter++;
        //         if (counter >= THRESHOLD)
        //         {
        //             LED_blink(LONG_DELAY);
        //             while (~SWITCH_PORT.IN & SWITCH_bm) /* wait until PB2 is pulled to VDD */
        //             {
        //                 ;
        //             }
        //             break;
        //         }
        //     }
        //     if (counter < THRESHOLD)
        //     {
        //         LED_blink(SHORT_DELAY);
        //     }
        //     counter = 0;
        // }
    }
}
