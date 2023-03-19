#ifndef NFC_H
#define	NFC_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdbool.h>
#include <stdint.h>
    
#define NFC_NO_E2 (0x00)
#define NFC_E2 (0x08)



#define PASSWORD_VALIDATION_PRESENT 0x09
#define PASSWORD_VALIDATION_CHANGE 0x07




// Miscelaneouc Registers (All E2=1)
#define NFC_REG_DSFID       0x0012
#define NFC_REG_AFI         0x0013
#define NFC_REG_MEM_SIZE    0x0014
#define NFC_REG_BLK_SIZE    0x0016
#define NFC_REG_IC_REF      0x0017
#define NFC_REG_UID         0x0017
#define NFC_REG_IC_REV      0x0020
#define NFC_REG_I2CPWD      0x9000





// ----------------------------- GPO  -----------------------
/**
 * GPO - GPO Control
 * Controls which events are sent over the GPO pin
 * 
 * E2 Set for Reg, not set for Dyn
 **/ 
#define NFC_REG_GPO             0x0000
#define NFC_REG_GPO_CTRL_Dyn    0x2000

#define NFC_GPO_RF_USER_bm      0x80
#define NFC_GPO_RF_ACTIVITY_bm  0x40
#define NFC_GPO_RF_INTERRUPT_bm 0x20
#define NFC_GPO_FIELD_CHANGE_bm 0x10 /* Factory Default */
#define NFC_GPO_PUT_MSG_bm      0x08
#define NFC_GPO_GET_MSG_bm      0x04
#define NFC_GPO_RF_WRITE_bm     0x02
#define NFC_GPO_ENABLED_bm      0x01 /* Factory Default */


/**
 * @brief IT_TIME 
 * Pulse Time for GPO events.
 */
// Times are +/- 2 uSec, and the values shown here are rounded
#define NFC_REG_IT_TIME     0x0001
typedef enum {
    NFC_GPO_PULSE_DURATION_301_US = (0b000) << 5,
    NFC_GPO_PULSE_DURATION_263_US = (0b100) << 5,
    NFC_GPO_PULSE_DURATION_226_US = (0b010) << 5,
    NFC_GPO_PULSE_DURATION_185_US = (0b110) << 5, /* Factory Default */
    NFC_GPO_PULSE_DURATION_150_US = (0b001) << 5,
    NFC_GPO_PULSE_DURATION_113_US = (0b101) << 5,
    NFC_GPO_PULSE_DURATION_75_US  = (0b011) << 5,
    NFC_GPO_PULSE_DURATION_37_US  = (0b111) << 5
} nfc_gpo_pulse_duration_t;


/**
 * @brief IT_STS_Dyn
 * Interruption Status - What caused the GPO pulse
 */
#define NFC_REG_IT_STS_Dyn  0x2005
#define NFC_INTERRUPT_USER_bm 0x01
#define NFC_INTERRUPT_RF_ACTIVITY_bm 0x02
#define NFC_INTERRUPT_RF_INTERRUPT_bm 0x04
#define NFC_INTERRUPT_FIELD_FALLING_bm 0x08
#define NFC_INTERRUPT_FIELD_RISING_bm 0x10
#define NFC_INTERRUPT_RF_PUT_MSG_bm 0x20
#define NFC_INTERRUPT_RF_GET_MSG_bm 0x40
#define NFC_INTERRUPT_RF_WRITE_bm 0x80



// ----------------------------- ENERGY HARVESTING -----------------------
/**
 * @brief EH_MODE
 * Determines how/if Energy Harvesting is done
 */
#define NFC_REG_EH_MODE     0x0002

#define NFC_EH_MODE_FORCED 0x00
#define NFC_EH_MODE_ON_DEMAND 0x01 /* Factory Default */

#define NFC_REG_EH_CTRL_Dyn 0x2002
#define NFC_EH_ENABLED_bm 0x01
#define NFC_EH_STATUS_ENABLED_bm 0x02
#define NFC_EH_STATUS_FIELD_ON_bm 0x04
#define NFC_EH_STATUS_VCC_ON_bm 0x08


// ----------------------------- RF Control -----------------------
/**
 * @brief RF_MNGT
 * Whether RF is enabled and/or sleeping. 
 */

#define NFC_REG_RF_MNGT     0x0003
#define NFC_REG_RF_CTRL_Dyn 0x2003

#define NFC_RF_DISABLE_bm 0x01
#define NFC_RF_SLEEP_bm 0x02




// ----------------------------- Access Control -----------------------
#define NFC_REG_ENDA1       0x0005
#define NFC_REG_ENDA2       0x0007
#define NFC_REG_ENDA3       0x0009
#define NFC_REG_RFA1SS      0x0004
#define NFC_REG_RFA2SS      0x0006
#define NFC_REG_RFA3SS      0x0008
#define NFC_REG_RFA4SS      0x000A
#define NFC_REG_I2CSS       0x000B

// I2C Security session status
#define NFC_REG_I2C_SSO_Dyn 0x2004

// RF Access Ctonrol - Two fields - The bottom bits are access control
typedef enum {
    NFC_RF_ACCESS_PWD_DISABLED = 0, /* Factory default */
    NFC_RF_ACCESS_PWD1,
    NFC_RF_ACCESS_PWD2,
    NFC_RF_ACCESS_PWD3,
} nfc_rf_access_passwd_source_t;

// Section 1 is treated slightly differently (Read always allowed, no matter this setting)
// these are bit shifted by 2 bits
#define NFC_RF_ACCESS_PROT_RW                      0 /* Factory default */
#define NFC_RF_ACCESS_PROT_READ_ALWAYS_WRITE_AUTH  0x04
#define NFC_RF_ACCESS_PROT_RW_AUTH                 0x08
#define NFC_RF_ACCESS_PROT_READ_AUTH               0x0C

// Section 1 is again slightly differet (Read always allowed)
// for i2c these are all combined into 1 uint8_t (each section shifted a progressive 2 bits)
#define NFC_I2C_ACCESS_PROT_A1_WRITE_AUTH_bm 0x01
#define NFC_I2C_ACCESS_PROT_A1_READ_AUTH_bm 0x02 /* Ignored */
#define NFC_I2C_ACCESS_PROT_A2_WRITE_AUTH_bm 0x04
#define NFC_I2C_ACCESS_PROT_A2_READ_AUTH_bm 0x08
#define NFC_I2C_ACCESS_PROT_A3_WRITE_AUTH_bm 0x10
#define NFC_I2C_ACCESS_PROT_A3_READ_AUTH_bm 0x20
#define NFC_I2C_ACCESS_PROT_A4_WRITE_AUTH_bm 0x40
#define NFC_I2C_ACCESS_PROT_A4_READ_AUTH_bm 0x80


// LOCK_CCFILE
#define NFC_REG_LOCK_CCFILE 0x000C
#define NFC_LOCK_LCKBCK0 0x01
#define NFC_LOCK_LCKBCK1 0x02

// LOCK_CFG
#define NFC_REG_LOCK_CFG    0x000F
#define NFC_LOCK_CFG_LOCKED 0x01


#define NFC_REG_LOCK_DSFID  0x0010
#define NFC_REG_LOCK_AFI    0x0011


// ----------------------------- Fast Mode Mailbox -----------------------

// MB_MODE
#define NFC_REG_MB_MODE     0x000D
#define NFC_MB_MODE_FORBIDDEN 0x00
#define NFC_MB_MODE_ENABLED 0x01

// MB_WDG
#define NFC_REG_MB_WDG      0x000E
#define NFC_MB_WDG_DISABLE 0x00
#define NFC_MB_WDG_30_MS 0x01
#define NFC_MB_WDG_60_MS 0x02
#define NFC_MB_WDG_120_MS 0x04


// MB_CTRL_Dyn
#define NFC_REG_MB_CTRL_Dyn 0x2006
// Fast mode mailbox
#define NFC_MB_CTRL_ENABLE_bm 0x01
#define NFC_MB_CTRL_HOST_PUT_MSG_bm 0x02
#define NFC_MB_CTRL_RF_PUT_MSG_bm 0x04

// MB_CTRL
#define NFC_MB_CTRL_HOST_MISS_MSG_bm 0x10
#define NFC_MB_CTRL_RF_MISS_MSG_bm 0x20
#define NFC_MB_CTRL_HOST_CURRENT_MSG_bm 0x40
#define NFC_MB_CTRL_RF_CURRENT_MSG_bm 0x80


// Fast mode message length
#define NFC_REG_MB_LEN_Dyn  0x2007
// The Mailbox itself.
#define NFC_REG_MB_dyn      0x2008


// -----------------------------------------------------------------------


typedef struct {
    // Requires Security Session
    uint8_t gpo;
    nfc_gpo_pulse_duration_t it_time; // Interupt time.
    uint8_t eh_mode;
    uint8_t rf_mngt;
    uint8_t rfa1ss; // area_1_rf_access_protection;
    uint8_t enda1;
    uint8_t rfa2ss;
    uint8_t enda2;
    uint8_t rfa3ss;
    uint8_t enda3;
    uint8_t rfa4ss;
    uint8_t i2css; // i2c access protection
    uint8_t lock_ccfile;
    uint8_t mb_mode; // Fast transfer mode. 
    uint8_t mb_wdg; 
    uint8_t lock_cfg; 
    uint8_t lock_dsfid; 
    uint8_t lock_afi; 
    uint8_t dsfid; 
    uint8_t afi; 
    uint16_t memSizeH;
    uint16_t memSizeL;
    uint8_t blk_size; 
    uint8_t ic_ref; 
    uint8_t uid[8];
} nfc_regs_t;

typedef struct {
    uint8_t status;
    uint8_t len; // This is one lower than the actual size, so 0xFF == 256 bytes.
} nfc_fast_transfer_mode_t;


#define NFC_get_dyn(addr, out) NFC_read(NFC_NO_E2, addr, out, 1)
#define NFC_get_reg(addr, out) NFC_read(NFC_E2, addr, out, 1)
#define NFC_get_fast_mode_status(out) NFC_read(NFC_NO_E2, NFC_MB_CTRL_Dyn, out, sizeof(nfc_fast_transfer_mode_t))




//If defined, internal pullup resistors will be used
// #define TWI_ENABLE_PULLUPS
    
    /**
     * <b><FONT COLOR=BLUE>void</FONT> TWI_initHost(<FONT COLOR=BLUE>void</FONT>)</B>
     * 
     * This function initializes the TWI peripheral in Host Mode.
     */
    void NFC_initHost(void);
    

    /**
     * <b><FONT COLOR=BLUE>void</FONT> TWI_sendAndReadBytes(<FONT COLOR=BLUE>uint8_t</FONT> addr, <FONT COLOR=BLUE>uint8_t</FONT> regAddress,<FONT COLOR=BLUE>uint8_t</FONT>* data, <FONT COLOR=BLUE>uint8_t</FONT> len)</B>
     * @param uint8_t addr - Client Device Address
     * @param uint8_t regAddress - Address of Register to Read From
     * @param uint8_t* data - Where the bytes received should be stored
     * @param uint8_t len - Number of Bytes to Send 
     * 
     * Reads data from the specified address
     */
    bool NFC_write(uint8_t e2, uint16_t regAddress, uint8_t* data, uint8_t len);

    /**
     * <b><FONT COLOR=BLUE>void</FONT> TWI_sendAndReadBytes(<FONT COLOR=BLUE>uint8_t</FONT> addr, <FONT COLOR=BLUE>uint8_t</FONT> regAddress,<FONT COLOR=BLUE>uint8_t</FONT>* data, <FONT COLOR=BLUE>uint8_t</FONT> len)</B>
     * @param uint8_t addr - Client Device Address
     * @param uint8_t regAddress - Address of Register to Read From
     * @param uint8_t* data - Where the bytes received should be stored
     * @param uint8_t len - Number of Bytes to Send 
     * 
     * Reads a number of bytes from the specified register address.
     */
    bool NFC_read(uint8_t e2, uint16_t regAddress, void* data, uint8_t len);

    bool NFC_present_password(uint8_t pw[8], uint8_t op);


    
#ifdef	__cplusplus
}
#endif

#endif	/* TWI_HOST_H */
