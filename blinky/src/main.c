#include <avr/io.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>


#define LED_PORT PORTA
#define LED_PIN PIN2_bm
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)


static volatile uint8_t isrCount = 0;


static void printch(char ch) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {
    }
    USART0.TXDATAL = ch;
}
static inline void printHexDigit(uint8_t val) {
    printch(val + (val < 10 ? '0' : 'a'));
}

static void printHex(uint8_t val) {
    printch('0');
    printch('x');
    printHexDigit(val >> 4);
    printHexDigit(val & 0x0F);
    printch('\r');
    printch('\n');
}


static void print(const char *str) {
    for (char *ch = (char *) str; *ch; ch++) {
        printch(*ch);
    }
    printch('\r');
    printch('\n');

}

static void uart_init() {
    // Use Alternate Pins, as the XCK and XDIR pins clash with uart. 
    PORTMUX.CTRLB = PORTMUX_USART0_bm;
    // Alternate TX pin is PA1
    PORTA.DIRSET = PIN1_bm;
    PORTA.OUTSET = PIN1_bm;
    USART0.BAUD = USART0_BAUD_RATE(115200);
    USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc |  USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc | USART_SBMODE_1BIT_gc;
    USART0.CTRLB = USART_TXEN_bm;
}


void nfc_reset_init() {
    // We use PC0 as our interrupt pin
    // Set it up to have pull up and interrupt on Falling 
    PORTC.DIRCLR = PIN0_bm;
    PORTC.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
}


ISR(PORTC_PORT_vect) {
    if (PORTC.INTFLAGS & PIN0_bm) {
        PORTC.INTFLAGS = PIN0_bm; // Clear the interrupt flag.

        // initiate a reset to the bootloader. 
        // RSTCTRL.SWRR = RSTCTRL_SWRE_bm;
        isrCount++;
    }
    if (PORTC.INTFLAGS & PIN1_bm) {
        PORTC.INTFLAGS = PIN1_bm; // Clear the interrupt flag.
    }
}

int main(void){
    // Set up the LED to be on.
    LED_PORT.DIRSET = LED_PIN;
    LED_PORT.OUTCLR = LED_PIN;

    // Enable Switch on PC1, with an interrupt on falling edge.
    PORTC.DIRCLR = PIN1_bm;
    PORTC.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;

    uart_init();
    print("Hello World");
    printHex(NVMCTRL.CTRLB);

    nfc_reset_init();
    PORTC.INTFLAGS = 0xFF; // Remove all existing interrupts.
    sei(); // Turn on interrupts (needed to allow for reset)

    uint8_t last = PORTC.IN & PIN1_bm;
    for(uint8_t count = 0;;count++) {
        _delay_ms(1);        
        if (count ==0) {
            LED_PORT.OUTTGL = LED_PIN;
        }

        if (isrCount > 0) {
            print("ISR");
            isrCount--;
        }

        uint8_t now = PORTC.IN & PIN1_bm;
        if (now != last) {
            last = now;
            print("Button");
            printHex(now);
        }
    }    
    __builtin_unreachable ();
}