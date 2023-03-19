#include <avr/io.h>
#include <stdbool.h>
#include <stdint.h>
// Needed to allow variable amount of delay.
// #define __DELAY_BACKWARD_COMPATIBLE__
#include <util/delay.h>

#include "nfc.h"

#define LED_PORT PORTA
#define LED_PIN PIN2_bm

// Default device address of 55h results in AAh default I²C write address and ABh default I²C read address

#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

// Constants for app locations.
#define BOOTEND_FUSE                (0x04)
#define BOOT_SIZE                   (BOOTEND_FUSE * 0x100)
#define MAPPED_APPLICATION_START    (MAPPED_PROGMEM_START + BOOT_SIZE)
#define MAPPED_APPLICATION_SIZE     (MAPPED_PROGMEM_SIZE - BOOT_SIZE)

typedef void (*const app_t)(void);


/*
 * Separate function for doing nvmctrl stuff.
 * It's needed for application to do manipulate flash, since only the
 *  bootloader can write or erase flash, or write to the flash alias areas.
 * Note that this is significantly different in the details than the
 *  do_spm() function provided on older AVRs.  Same "vector", though.
 *
 * How it works:
 * - if the "command" is legal, write it to NVMCTRL.CTRLA
 * - if the command is not legal, store data to *address
 * - wait for NVM to complete
 *
 * For example, to write a flash page:
 * Copy each byte with
 *   do_nvmctrl(flashOffset+MAPPED_PROGMEM_START, 0xFF, *inputPtr);
 * Erase and write page with
 *   do_nvmctrl(0, NVMCTRL_CMD_PAGEERASEWRITE_gc, 0);
 */
// static void do_nvmctrl(uint16_t address, uint8_t command, uint8_t data)  __attribute__ ((used));
// static void do_nvmctrl (uint16_t address, uint8_t command, uint8_t data) {
//   if (command <= NVMCTRL_CMD_gm) {
//     _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, command);
//     while (NVMCTRL.STATUS & (NVMCTRL_FBUSY_bm|NVMCTRL_EEBUSY_bm))
//       ; // wait for flash and EEPROM not busy, just in case.
//   } else {
//     *(uint8_t *)address = data;
//   }
// }

static void printch(char ch) {
    while (!(USART0.STATUS & USART_DREIF_bm)) {
    }
    USART0.TXDATAL = ch;
}
static inline void printHexDigit(uint8_t val) {
    val &= 0x0F;
    if (val < 10) {
        printch(val + '0');
    } else {
        printch(val - 10 + 'a');
    }
}

static void printHex(uint8_t val) {
    printch('0');
    printch('x');
    printHexDigit(val >> 4);
    printHexDigit(val & 0x0F);
    printch('\r');
    printch('\n');
}



void print(const char *str) {
    for (char *ch = (char *) str; *ch; ch++) {
        printch(*ch);
    }
    printch('\r');
    printch('\n');

}

// Fast blink the LED to indicate a bootloader error.  Device will need to be reset (via NFC) to achieve anything else.
static void show_error() {
    for (;;) {
        LED_PORT.OUTTGL = LED_PIN;
        _delay_ms(500);
    }
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

static nfc_regs_t nfc;

// Help obtained from https://ww1.microchip.com/downloads/en/Appnotes/AN2634-Bootloader-for-tinyAVR-and-megaAVR-00002634C.pdf
// Bootloader is compiled with -nostartfiles, and has no ISR table.
// When compiling target application use -Wl,--section-start=.text=0x400 to offset code
__attribute__((naked)) __attribute__((section(".ctors"))) void boot(void){

    /* Initialize system for C support */
    asm volatile("clr r1");
    uart_init();
    print("Boot");
    printHex(MAPPED_PROGMEM_PAGE_SIZE);
    // Initialise the WDT

    // Disable Read from BOOT from APP, and disable writing to APP


    // Make PB0 an Output
    LED_PORT.DIRSET = LED_PIN;
    // Turn on the LED to indicate that we're in the Bootloader
    LED_PORT.OUTCLR = LED_PIN;



    // Initialise I2C
    NFC_initHost();

    // Check to see if tag is configured - configure it if necessary. This could be managed by device address.
    // Start first read from well known address.  If we get a NAK, check the default address and reconfigure.
    // This won't work for the NXP tag, as it has a fixed address.  Alternate mechanism - Check value in EEPROM on tag,
    // if it is set to a magic value, then it is configured.  Set the magic value as part of configuration.

    // Read in all sT25DV configuration registers
    bool success = NFC_read(NFC_E2, 0x0000, &nfc, sizeof(nfc));
    // Turn off TWI
    TWI0.MCTRLA = 0;



    print("GPO");
    printHex(nfc.gpo);
    print("IT Time");
    printHex(nfc.it_time);
    print("EH Mode");
    printHex(nfc.eh_mode);

    // By default, the tag should be configured to send out a GPIO pulse whenever a block is written to
    // its SRAM buffer.  During firmware streaming, this should be disabled and polling used, as it would reset the device.

    // Check to see if there's a block for us on the NFC card
    // If yes, go into download mode
    // Otherwise, boot as normal.
    if (!success) {
        print("Error");
        show_error();
    }

    // Jump to the "App", whatever that is - Initial flash includes a dummy one, but this bootloader may overwrite it
    print("=>App");

    // Wait for UART to Flush
    while (!(USART0.STATUS & USART_TXCIF_bm)) {
    }
    USART0.STATUS = USART_TXCIF_bm; // Clear the Transmit complete flag
    _delay_ms(1); // That only flushed the buffer.  It also needs time to write the characters from the device.

    // Turn off the UART
    // TODO once disabled, the UART doesn't seem to be able to be re-enabled. 
    // USART0.CTRLB = 0; // This turns the TX pin back into an input, no matter what it was before. 

    // Turn the LED Back off again
    LED_PORT.OUTSET = LED_PIN;
    LED_PORT.DIRCLR = LED_PIN;


    // Turn off ability to write to application code (Only the Bootloader can write)
    // TODO doesn't seem to work.
    NVMCTRL.CTRLB = NVMCTRL_BOOTLOCK_bm;

    // app_t app = (app_t)(BOOT_SIZE / sizeof(app_t));
    // app();

    __asm__ __volatile__(
        "  jmp 0x8400\n"
    );
}

/*
Writing to Page
	 // Start programming at start for application section
	 // Subtract MAPPED_PROGMEM_START in condition to handle overflow on large flash sizes
	uint8_t *app_ptr = (uint8_t *)MAPPED_APPLICATION_START;
	while(app_ptr - MAPPED_PROGMEM_START <= (uint8_t *)PROGMEM_END) {
		// Receive and echo data before loading to memory
		uint8_t rx_data = twi_receive();
		twi_send(rx_data);

		// Incremental load to page buffer before writing to Flash
		*app_ptr = rx_data;
		app_ptr++;
		if(!((uint16_t)app_ptr % MAPPED_PROGMEM_PAGE_SIZE)) {
			// Page boundary reached, Commit page to Flash 
			_PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
			while(NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);

			toggle_status_led();
		}
	}
*/