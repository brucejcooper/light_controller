#include "nfc.h"

#include <avr/io.h>
#include <stdbool.h>
#include <string.h>

#define TWI_READ 0x01
#define TWI_WRITE 0x00

#define TWI_IS_CLOCKHELD() TWI0.MSTATUS & TWI_CLKHOLD_bm
#define TWI_IS_BUSERR() TWI0.MSTATUS & TWI_BUSERR_bm
#define TWI_IS_ARBLOST() TWI0.MSTATUS & TWI_ARBLOST_bm

#define CLIENT_NACK() TWI0.MSTATUS & TWI_RXACK_bm
#define CLIENT_ACK() !(TWI0.MSTATUS & TWI_RXACK_bm)

#define TWI_IS_BUSBUSY() ((TWI0.MSTATUS & TWI_BUSSTATE_BUSY_gc) == TWI_BUSSTATE_BUSY_gc)
//#define TWI_IS_BAD() ((TWI_IS_BUSERR()) | (TWI_IS_ARBLOST()) | (CLIENT_NACK()) | (TWI_IS_BUSBUSY()))


#define TWI_SCL_FREQ 100000
#define TWI_BAUD ((((float) F_CPU / (float) TWI_SCL_FREQ) - 10)/2)

#define NFC_ADDR (0x53)
#define NFC_ADDR_SHIFTED (NFC_ADDR << 1)

static bool isTWIBad(void)
{
    //Checks for: NACK, ARBLOST, BUSERR, Bus Busy
    if ((((TWI0.MSTATUS) & (TWI_RXACK_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm)) > 0)
            || (TWI_IS_BUSBUSY()))
    {
        return true;
    }
    return false;
}

void twi_wait() {
    while (!((TWI_IS_CLOCKHELD()) || (TWI_IS_BUSERR()) || (TWI_IS_ARBLOST()) || (TWI_IS_BUSBUSY()))) {

    }
}


static void TWI_initPins(void)
{
    //PB1/PB2
    //Output I/O - Default to High and out.
    PORTB.OUTSET = PIN0_bm | PIN1_bm;
    PORTB.DIRSET = PIN0_bm | PIN1_bm;

#ifdef TWI_ENABLE_PULLUPS
    //Enable Pull-Ups
    PORTB.PIN0CTRL = PORT_PULLUPEN_bm;
    PORTB.PIN1CTRL = PORT_PULLUPEN_bm;
#endif
}


void NFC_initHost(void)
{        
    //Setup TWI I/O
    TWI_initPins();
    
    //Standard 100kHz TWI, 4 Cycle Hold, 50ns SDA Hold Time
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    
    //Enable Run in Debug
    TWI0.DBGCTRL = TWI_DBGRUN_bm;
    
    //Clear MSTATUS (write 1 to flags). BUSSTATE set to idle
    TWI0.MSTATUS = TWI_RIF_bm | TWI_WIF_bm | TWI_CLKHOLD_bm | TWI_RXACK_bm |
            TWI_ARBLOST_bm | TWI_BUSERR_bm | TWI_BUSSTATE_IDLE_gc;
    
    //Set for 100kHz.
    TWI0.MBAUD = TWI_BAUD;
    
    //[No ISRs] and Host Mode
    TWI0.MCTRLA = TWI_ENABLE_bm;
}


bool _writeByteToTWI(uint8_t data) {
    //Write a byte
    TWI0.MDATA = data;
    
    //Wait...
    twi_wait();
    return CLIENT_ACK();
}


static bool _startTWI(uint8_t e2, uint16_t regAddress)
{
    //If the Bus is Busy
    if (TWI_IS_BUSBUSY())
    {
        return false;
    }
    
    // Start command is always a write.
    TWI0.MADDR = NFC_ADDR_SHIFTED | e2;

    //Wait...
    twi_wait();
                
    if (isTWIBad()) {
        return false;
    }

    //Write register address
    if (!_writeByteToTWI(regAddress >> 8)) {
        return false;
    }
    if (!_writeByteToTWI(regAddress)) {
        return false;
    }
    
    //TWI Started
    return true;
}





bool NFC_write(uint8_t e2, uint16_t regAddress, uint8_t* data, uint8_t len) {
    if (!_startTWI(e2, regAddress)) {
        goto error;
    }
       
    for (uint8_t count = 0; count < len; count++) {
        if (!_writeByteToTWI(data[count])) {
            goto error;
        }
    }
    return true;
error:
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    return false;
}



bool NFC_read(uint8_t e2, uint16_t regAddress, void* vdata, uint8_t len) {
    //Address Client Device (Write)
    if (!_startTWI(e2, regAddress)) {
        goto error;
    }
            
    //Restart the TWI Bus in READ mode
    TWI0.MADDR |= TWI_READ;
    TWI0.MCTRLB = TWI_MCMD_REPSTART_gc;
    
    //Wait...
    twi_wait();
    
    if (isTWIBad()) {
        goto error;
    }
    
    //Read the data from the client
    //Release the clock hold
    TWI0.MSTATUS = TWI_CLKHOLD_bm;
    
    // TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
    for (uint8_t bCount = 0;  bCount < len;) {
        //Wait...
        twi_wait();
        
        //Store data
        ((uint8_t *) vdata)[bCount++] = TWI0.MDATA;        
        if (bCount < len) {
            //If not done, then ACK and read the next byte
            TWI0.MCTRLB = TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc;
        }
    }
    
    //NACK and STOP the bus
    TWI0.MCTRLB = TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
    return true;
error:
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    return false;
}


bool NFC_present_password(uint8_t pw[8], uint8_t op) {
    // Presenting password is done via 8 password bytes, the op (present or set) then the password again.
    uint8_t buf[17];
    memcpy(buf, pw, 8);
    buf[8] = op;
    memcpy(buf+9, pw, 8);

    // After successfull compare, I2C_SSO_dyn register set to 0x01 - set to 0x00 otherwise.

    return NFC_write(NFC_E2, 0x0900, buf, 17);
}

