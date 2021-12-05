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
#include <util/delay.h>
#include <stdio.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit)) // clear bit
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))  // set bit

/* ATTiny85 IO pins
             ___^___
           -|PB5 VCC|-
LED        -|PB3 PB2|-
serial out -|PB4 PB1|-
           -|GND PB0|-
             -------
*/

/* prototypes */
// main routines
void setup(void);
void loop(void);
int main(void);
// misc routines
void init_printf(void);
int serial_putc(char c, FILE *file);
void serial_write(uint8_t tx_byte);
uint64_t millis(void);

/* some vars */
volatile uint64_t _millis    = 0;
volatile uint16_t _1000us    = 0;
uint64_t old_millis = 0;

// must be volatile (change and test in main and ISR)
volatile uint8_t tx_buzy = 0;
volatile uint8_t bit_index;
volatile uint8_t _tx_buffer; 

/*** ISR ***/


typedef enum  {
    TURN_ON,
    TURN_OFF,
} timer_mode_t;

volatile uint8_t last_reading;
volatile int16_t ticks_till_off = 0;
volatile timer_mode_t timer_mode = TURN_ON;
volatile float duty = 0.5;
// Start with a dummy value for estimated_zc.
volatile uint8_t estimated_zc_after_drop = (626/2 / 8);



// ISR(ADC_vect) {
//     // copy the reading into our variable.
//     uint8_t val = ADCH;
//     if (val == 0) {
//         if (last_reading > 0) {           
//             // We can mess with the timers becase the clock isn't running right now.
//             OCR0A = estimated_zc_after_drop;
//             GTCCR = (1 << PSR0); // Clear the prescaler - it should be off at this time.
//             TCNT0 = 0; // Start from 0
//             TCCR0B = (1 << CS01) | (1 << CS00); // Start timer clock at /64
//             timer_mode = TURN_ON;
//         }
//     } else {
//         if (last_reading == 0) {
//             // We overflowed at the previous zc_after_drop, which rolled us back to 0.  So add thhat to TCNT0 then halve it to get the estimated zc.
//             estimated_zc_after_drop = (estimated_zc_after_drop + TCNT0)/2;
//         }
//     }
//     last_reading = val;
// }



// ISR(TIM0_COMPA_vect) {
//     // Timer has gone off.  
//     if (timer_mode == TURN_ON) {
//         // Turn on PB1, turn off PB4
//         sbi(PORTB, PB1);
//         timer_mode = TURN_OFF;
//         ticks_till_off = duty * (10000/8);
//         OCR0A = ticks_till_off > 255 ? 255 : ticks_till_off;
//     } else {
//         ticks_till_off -= 256;
//         if (ticks_till_off <= 0) {
//             cbi(PORTB, PB1);
//             TCCR0B = 0; // Disable the clock, we're back to waiting for ZCD.            
//         } else {
//             OCR0A = ticks_till_off > 255 ? 255 : ticks_till_off;
//         }

//     //     ticks_till_off -= TCNT0;
//     //     if (ticks_till_off == 0) {
//     //         cbi(PORTB, PB1);
//     //         TCCR0B = 0; // Disable the clock, we're back to waiting for ZCD.
//     //     } else {
//     //         if (ticks_till_off < 255) {
//     //             TCNT0 = ticks_till_off;
//     //         }
//     //     }
//     }
// }


ISR(PCINT0_vect) {
	PORTB ^= (1 << PB4); // Toggle PB4
}


// // compare match interrupt service for OCR0A
// // call every 103us
// ISR(TIM0_COMPA_vect) { 
//   // software UART
//   // send data
//   if (tx_buzy) {
//     if (bit_index == 0) {
//       // start bit (= 0)
//       cbi(PORTB, PB4);
//     } else if (bit_index <=8) {
//       // LSB to MSB
//       if (_tx_buffer & 1) {
//         sbi(PORTB, PB4);
//       } else {
//         cbi(PORTB, PB4);
//       }
//       _tx_buffer >>= 1;        
//     } else if (bit_index >= 9) {
//       // stop bit (= 1)
//       sbi(PORTB, PB4);
//       tx_buzy = 0;
//     }
//     bit_index++;
//   }
//   // millis update
//   _1000us += 103;
//   while (_1000us > 1000) {
//     _millis++;
//     _1000us -= 1000;
//   }
// }

// /*** UART routines ***/
// // send serial data to software UART, block until UART buzy
// void serial_write(uint8_t tx_byte) {
//   while(tx_buzy);
//   bit_index  = 0;
//   _tx_buffer = tx_byte;
//   tx_buzy = 1;
// }
// /*** connect software UART to stdio.h ***/
// void init_printf() {
//   fdevopen(&serial_putc, 0);
// }

// int serial_putc(char c, FILE *file) {
//   serial_write(c);
//   return c;
// }

// /*** misc routines ***/

// // safe access to millis counter
// uint64_t millis() {
//   uint64_t m;
//   cli();
//   m = _millis;
//   sei();
//   return m;
// }

/*** main routines ***/

// How to do accurate PWM with ZCD (drop to zero), but not an accurate ZCD signal.
// Set clock divider to 64 (8us per tick) - 125 ticks for a millisecond
// 78 ticks to get to 624 us (average width of zero period) - 39ish to get to middle.
// 5 ms (50% duty) in 8us ticks == 625 - get it to roll over twice, then to 113 - Turn it back off again.
// Could we change the divider on the fly?
// could we leave it at /8, and get it to roll over more often? rollovers would fit in a uint16

// ISR(ANA_COMP_vect){
//     PORTB^=(1<<PB4);    //toggle OUTPUT.
// }




void setup(void) {
    // Make DDB an input.
    cbi(DDRB, PB3); 
    // DISABLE pull-up - Its driven externally.
    cbi(PORTB, PB3);
    // Turn on edge interrupts for PB3, on both rising and faling.
    sbi(PCMSK, PCINT3);
    sbi(GIMSK, PCIE);



    // Use VDD as Ref (consider using 2.56), and ADC MUX of PB3, and right shift
    // ADMUX = (0 << REFS2) | (0 << REFS1) | (0 << REFS0) | (1 << ADLAR) | (0 << MUX2) | (1 << MUX1) | (1 << MUX0); 
    // // Turn ADC on, do it automatically, with an interupt, and a clock divider of 64 (9000ish samples/sec)
    // ADCSRA =  (1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIE) | (1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0);


    sbi(DDRB, PB4); // PB4 is output
    cbi(PORTB, PB4);


    /* interrup setup */
    // call ISR(TIM0_COMPA_vect) every 103us (for 9600 bauds)
    // set CTC mode : clear timer on comapre match
    // -> reset TCNTO (timer 0) when TCNTO == OCR0A

    // sbi(TCCR0A, WGM01); // CTC Mode.
    // Don't set the prescaler yet.  We want it not running
    // // compare register A at 103 us
    // OCR0A = 103;
    // // interrupt on compare match OCROA == TCNT0
    // sbi(TIMSK, OCIE0A);
    // Enable global interrupts
    sei();
}

/*
  Arduino like
*/
int main(void) {
  setup();
  uint8_t last = 0;
  for(;;) {
      // Check duty,  
      // if 0% or 100%, set the output appropriately, turn off timers and ZCD, then SLEEP, awaiting Interrupt on Power button or protocol - Pity you can't Power down (two level interrupts)
      // If anything else, turn on ZCD, then SLEEP. 

  }
};