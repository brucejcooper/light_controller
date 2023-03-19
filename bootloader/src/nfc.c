#include "nfc.h"

#include <avr/io.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>

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
    TWI0.MBAUD = 15; // TWI_BAUD;
    
    //[No ISRs] and Host Mode
    TWI0.MCTRLA = TWI_ENABLE_bm;
}


static inline void waitForWrite() {
    while (!(TWI0.MSTATUS & TWI_WIF_bm)) {}
}

static inline void waitForRead() {
    while (!(TWI0.MSTATUS & TWI_RIF_bm)) {}
}

static inline bool gotAckFromWrite() {
   // Returns true if we got an ack and there were no bus errors or loss of arbitration.
   // For the purposes of this bootloader, we don't differentiate between a nack and
   // a bus error or loss of abritration.
   return (TWI0.MSTATUS & (TWI_RXACK_bm | TWI_ARBLOST_bm | TWI_BUSERR_bm)) == 0;
}


bool _writeByteToTWI(uint8_t data) {
    //Write a byte
    TWI0.MDATA = data;
    waitForWrite();
    return gotAckFromWrite();
}





static bool _startTWI(uint8_t e2, uint16_t regAddress)
{
    //If the Bus is Busy
    if (TWI_IS_BUSBUSY())
    {
        return false;
    }

    // Start command is always a write.
    TWI0.MADDR = NFC_ADDR_SHIFTED | TWI_WRITE | e2;
    waitForWrite();

    // Check to see that we got an ACK.
    if (!gotAckFromWrite()) {
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
    bool result = false;
    if (!_startTWI(e2, regAddress)) {
        goto cleanup;
    }
    result = true;
    while (result && len--) {
        result = _writeByteToTWI(*data++);
    }
cleanup:
    TWI0.MCTRLB = TWI_MCMD_STOP_gc;
    return result;
}



bool NFC_read(uint8_t e2, uint16_t regAddress, void* vdata, uint8_t len) {
    uint8_t *dptr = (uint8_t *) vdata;

    //Address Client Device (Write)
    if (!_startTWI(e2, regAddress)) {
        goto error;
    }
            
    //Restart the TWI Bus, and send the read address
    TWI0.MCTRLB = TWI_MCMD_REPSTART_gc;
    TWI0.MADDR |= TWI_READ;
    TWI0.MCTRLB = len ? TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc :  TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;

    // TODO how do we tell if the read address wasn't acknowledged?  It doesn't set WIF after writing the address - it just goes immediately into reading.
    
    // TWI device will automatically clock one byte of data in after sending the read address.  
    while (len--) {        
        // Set action for next byte. We ack all but the last one, then issue a STOP.
        waitForRead();

        // At this point, the clock is held, waiting for our response. 

        //Store data - This will clear RIF and CLKHOLD
        *(dptr++) = TWI0.MDATA;        

        // A byte will have been read in at this point, but we need to respond.  This will
        // release the clock hold and clear RIF
        TWI0.MCTRLB = len ? TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc :  TWI_ACKACT_NACK_gc | TWI_MCMD_STOP_gc;
    }

    // Wait for last read to be completed (Bus to return to idle) - Not 100% sure this is the right move, but it works.
    while ((TWI0.MSTATUS & TWI_BUSSTATE_gm) != TWI_BUSSTATE_IDLE_gc) { }
    
    if (TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {
        goto error;
    }
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
    return NFC_write(NFC_E2, NFC_REG_I2CPWD, buf, 17);
}

