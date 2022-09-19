/******************************************************************************/
// dali.h                                                                  
// definitions for DALI and clock                                            
// Customized Definitions
//      PHYS_MIN_LEVEL
//      MCLK_KHZ
//      SMCLK_KHZ
//      WDT_CYCLES
//      FADE_INTERVAL
//      CONFIG_TIMEOUT      
//      DAPC_TIMEOUT        
//      INITIALISE_SEC      
//      POWER_ON_TIME             
//                                                     
// Chris Sterzik                                                               
// Texas Instruments                                                         
// Built with CCS Version: 5.20
//
// Revision 1.1    09/17/2012
/******************************************************************************/

// THIS PROGRAM IS PROVIDED "AS IS". TI MAKES NO WARRANTIES OR
// REPRESENTATIONS, EITHER EXPRESS, IMPLIED OR STATUTORY,
// INCLUDING ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
// COMPLETENESS OF RESPONSES, RESULTS AND LACK OF NEGLIGENCE.
// TI DISCLAIMS ANY WARRANTY OF TITLE, QUIET ENJOYMENT, QUIET
// POSSESSION, AND NON-INFRINGEMENT OF ANY THIRD PARTY
// INTELLECTUAL PROPERTY RIGHTS WITH REGARD TO THE PROGRAM OR
// YOUR USE OF THE PROGRAM.
//
// IN NO EVENT SHALL TI BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
// CONSEQUENTIAL OR INDIRECT DAMAGES, HOWEVER CAUSED, ON ANY
// THEORY OF LIABILITY AND WHETHER OR NOT TI HAS BEEN ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGES, ARISING IN ANY WAY OUT
// OF THIS AGREEMENT, THE PROGRAM, OR YOUR USE OF THE PROGRAM.
// EXCLUDED DAMAGES INCLUDE, BUT ARE NOT LIMITED TO, COST OF
// REMOVAL OR REINSTALLATION, COMPUTER TIME, LABOR COSTS, LOSS
// OF GOODWILL, LOSS OF PROFITS, LOSS OF SAVINGS, OR LOSS OF
// USE OR INTERRUPTION OF BUSINESS. IN NO EVENT WILL TI'S
// AGGREGATE LIABILITY UNDER THIS AGREEMENT OR ARISING OUT OF
// YOUR USE OF THE PROGRAM EXCEED FIVE HUNDRED DOLLARS
// (U.S.$500).
//
// Unless otherwise stated, the Program written and copyrighted
// by Texas Instruments is distributed as "freeware".  You may,
// only under TI's copyright in the Program, use and modify the
// Program without any charge or restriction.  You may
// distribute to third parties, provided that you transfer a
// copy of this license to the third party and the third party
// agrees to these terms by its first use of the Program. You
// must reproduce the copyright notice and any other legend of
// ownership on each copy or partial copy, of the Program.
//
// You acknowledge and agree that the Program contains
// copyrighted material, trade secrets and other TI proprietary
// information and is protected by copyright laws,
// international copyright treaties, and trade secret laws, as
// well as other intellectual property laws.  To protect TI's
// rights in the Program, you agree not to decompile, reverse
// engineer, disassemble or otherwise translate any object code
// versions of the Program to a human-readable form.  You agree
// that in no event will you alter, remove or destroy any
// copyright notice included in the Program.  TI reserves all
// rights not specifically granted under this license. Except
// as specifically provided herein, nothing in this agreement
// shall be construed as conferring by implication, estoppel,
// or otherwise, upon you, any license or other right under any
// TI patents, copyrights or trade secrets.
/******************************************************************************/
#ifndef DALI_H_
#define DALI_H_

/* Global Varaible for DALI */
extern const unsigned char min_fast_fade_time;
extern const unsigned char gear_type;
extern const unsigned char possible_operating_modes;
extern const unsigned char features;
extern const unsigned char version_number;
extern const unsigned char extended_version_number;
extern const unsigned char device_type;
extern const unsigned char phys_min_lvl;
extern const unsigned char memory_bank_0[];

extern unsigned char power_on_level;
extern unsigned char actual_level;
//extern unsigned long int search_address;
extern unsigned char search_address_byte_h;
extern unsigned char search_address_byte_m;
extern unsigned char search_address_byte_l;
//extern unsigned long int random_address;
extern unsigned char random_address_byte_h;
extern unsigned char random_address_byte_m;
extern unsigned char random_address_byte_l;
extern unsigned char system_failure_level;
extern unsigned char min_level;
extern unsigned char max_level;
extern unsigned char fade_rate;  
extern unsigned char fade_time;
extern unsigned char fast_fade_time;
extern unsigned char short_address;
extern unsigned char group_0_7;
extern unsigned char group_8_f;
extern unsigned char scene[];
extern unsigned char status_information;
extern unsigned char data_transfer_register;
extern unsigned char data_transfer_register1;
extern unsigned char data_transfer_register2;
extern unsigned char failure_status;
extern unsigned char operating_mode;
extern unsigned char dimming_curve;

/* Additional global variables */
extern unsigned char target_level;
extern unsigned long fade_count;
extern unsigned int config_count;
extern unsigned long dapc_count;
extern unsigned long fade_step_size;
extern unsigned char *flash_ptr;
extern unsigned char flash_update_request;
extern unsigned long initialise_count;
extern unsigned char initialise_minute;
extern unsigned char idle_time;
extern unsigned char physical_selection;
extern unsigned char enable_device_type;
/*** Low level function prototypes ********************************************/
unsigned char TI_DALI_Match_Address(unsigned char);
void TI_DALI_Reset_Check(void);
unsigned char TI_DALI_Rx(unsigned char *rx_msg);
void TI_DALI_Tx(unsigned char);

extern unsigned char TI_DALI_Command(unsigned char*);
extern unsigned char TI_DALI_Special_Command(unsigned char*);

/*** Higher level Prototypes **************************************************/
extern void TI_DALI_Update_Callback(void);
extern void TI_DALI_Idle_Callback(void);
void TI_DALI_Transaction_Loop(void);
void TI_DALI_Init(unsigned int);
void TI_DALI_Flash_Update(unsigned int);
/*** Application Definitions **************************************************/
#define PHYS_MIN_LEVEL    1
#define MCLK_KHZ          8000        // Application MHZ frequency
#define SMCLK_KHZ         8000

#define WDT_CYCLES        8192

//#define DIV_SMCLK_32768   (WDT_MDLY_32)     /* SMCLK/32768  */
#define FADE_INTERVAL     (WDT_MDLY_8)      /* SMCLK/8192   */
//#define DIV_SMCLK_512     (WDT_MDLY_0_5)    /* SMCLK/512    */
//#define DIV_SMCLK_64      (WDT_MDLY_0_064)  /* SMCLK/64     */
// The following definitions are not scaled to the Clock frequency or the
// WDT interval.  In this example it is simply 8192/8000 or 1.024.
/* 100ms */
#define CONFIG_TIMEOUT      98
/* 200ms */
#define DAPC_TIMEOUT        195
/* 60000ms per minute */
#define INITIALISE_SEC      58594
/* 600 ms */
#define POWER_ON_TIME       586

/*** Clock Definitions ********************************************************/
// These definitions serve as the basis for the Manchester encoding and 
// specfic DALI timeout requirements. 

#define MCLK_uSEC_COUNT   ((unsigned long)MCLK_KHZ / 1000)
                          // # of MCLK cycles equivalent to 1 us 
#define MCLK_mSEC_COUNT   ((unsigned long)MCLK_KHZ)        
                          // # of MCLK cycles equivalent to 1 ms
#define MCLK_SEC_COUNT    ((unsigned long)MCLK_KHZ * 1000)
                          // # of MCLK cycles equivalent to 1 second

#define SMCLK_uSEC_COUNT  ((unsigned long)SMCLK_KHZ / 1000)
                          // # of SMCLK cycles equivalent to 1 us 
#define SMCLK_mSEC_COUNT  ((unsigned long)SMCLK_KHZ)
                          // # of SMCLK cycles equivalent to 1 ms
#define SMCLK_SEC_COUNT   ((unsigned long)SMCLK_KHZ * 1000)
                          // # of SMCLK cycles equivalent to 1 second                        
/******************************************************************************/
#define TRUE                1
#define FALSE               0
#define OFF                 0x00
#define SYSTEM_FAILURE      15
#define VALID_BYTES         2
#define NO_FADE             0
#define MASK                0xFF
/* 15 minutes */
#define INITIALISE_MIN      15
#define RANDOM_TIME         (INITIALISE_TIMEOUT/200)

#define FADE_RATE_1   ((unsigned long)(SMCLK_SEC_COUNT/358)/WDT_CYCLES)
#define FADE_RATE_2   ((unsigned long)(SMCLK_SEC_COUNT/253)/WDT_CYCLES)
#define FADE_RATE_3   ((unsigned long)(SMCLK_SEC_COUNT/179)/WDT_CYCLES)
#define FADE_RATE_4   ((unsigned long)(SMCLK_SEC_COUNT/127)/WDT_CYCLES)
#define FADE_RATE_5   ((unsigned long)(SMCLK_SEC_COUNT/89.4)/WDT_CYCLES)
#define FADE_RATE_6   ((unsigned long)(SMCLK_SEC_COUNT/63.3)/WDT_CYCLES)
#define FADE_RATE_7   ((unsigned long)(SMCLK_SEC_COUNT/44.7)/WDT_CYCLES)
#define FADE_RATE_8   ((unsigned long)(SMCLK_SEC_COUNT/31.6)/WDT_CYCLES)
#define FADE_RATE_9   ((unsigned long)(SMCLK_SEC_COUNT/22.4)/WDT_CYCLES)
#define FADE_RATE_A   ((unsigned long)(SMCLK_SEC_COUNT/15.8)/WDT_CYCLES)
#define FADE_RATE_B   ((unsigned long)(SMCLK_SEC_COUNT/11.2)/WDT_CYCLES)
#define FADE_RATE_C   ((unsigned long)(SMCLK_SEC_COUNT/7.9)/WDT_CYCLES)
#define FADE_RATE_D   ((unsigned long)(SMCLK_SEC_COUNT/5.6)/WDT_CYCLES)
#define FADE_RATE_E   ((unsigned long)(SMCLK_SEC_COUNT/4.0)/WDT_CYCLES)
#define FADE_RATE_F   ((unsigned long)(SMCLK_SEC_COUNT/2.8)/WDT_CYCLES)

#define FADE_200ms_1  ((unsigned int)358*20/100)
#define FADE_200ms_2  ((unsigned int)253*200/1000)
#define FADE_200ms_3  ((unsigned int)179*200/1000)
#define FADE_200ms_4  ((unsigned int)127*200/1000)
#define FADE_200ms_5  ((unsigned int)89.4*200/1000)
#define FADE_200ms_6  ((unsigned int)63.3*200/1000)
#define FADE_200ms_7  ((unsigned int)44.7*200/1000)
#define FADE_200ms_8  ((unsigned int)31.6*200/1000)
#define FADE_200ms_9  ((unsigned int)22.4*200/1000)
#define FADE_200ms_A  ((unsigned int)15.8*200/1000)
#define FADE_200ms_B  ((unsigned int)11.2*200/1000)
#define FADE_200ms_C  ((unsigned int)7.9*200/1000)
#define FADE_200ms_D  ((unsigned int)5.6*200/1000)
#define FADE_200ms_E  ((unsigned int)4.0*200/1000)
#define FADE_200ms_F  ((unsigned int)2.8*200/1000)

#define FADE_TIME_1   ((unsigned long)700*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_2   ((unsigned long)1000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_3   ((unsigned long)1400*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_4   ((unsigned long)2000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_5   ((unsigned long)2800*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_6   ((unsigned long)4000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_7   ((unsigned long)5700*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_8   ((unsigned long)8000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_9   ((unsigned long)11300*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_A   ((unsigned long)16000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_B   ((unsigned long)22600*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_C   ((unsigned long)32000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_D   ((unsigned long)45300*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_E   ((unsigned long)64000*WDT_CYCLES/SMCLK_KHZ)
#define FADE_TIME_F   ((unsigned long)90500*WDT_CYCLES/SMCLK_KHZ)

#define FAST_FADE_TIME_1  FAST_FADE_TIME_B  
                          //((unsigned long)25*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_2  FAST_FADE_TIME_B  
                          //((unsigned long)50*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_3  FAST_FADE_TIME_B  
                          //((unsigned long)75*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_4  FAST_FADE_TIME_B  
                          //((unsigned long)100*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_5  FAST_FADE_TIME_B  
                          //((unsigned long)125*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_6  FAST_FADE_TIME_B  
                          //((unsigned long)150*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_7  FAST_FADE_TIME_B  
                          //((unsigned long)175*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_8  FAST_FADE_TIME_B  
                          //((unsigned long)200*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_9  FAST_FADE_TIME_B  
                          //((unsigned long)225*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_A  FAST_FADE_TIME_B  
                          //((unsigned long)250*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_B  ((unsigned long)275*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_C  ((unsigned long)300*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_D  ((unsigned long)325*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_E  ((unsigned long)350*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_F  ((unsigned long)375*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_10 ((unsigned long)400*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_11 ((unsigned long)425*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_12 ((unsigned long)450*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_13 ((unsigned long)475*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_14 ((unsigned long)500*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_15 ((unsigned long)525*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_16 ((unsigned long)550*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_17 ((unsigned long)575*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_18 ((unsigned long)600*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_19 ((unsigned long)625*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_1A ((unsigned long)650*SMCLK_KHZ/WDT_CYCLES)
#define FAST_FADE_TIME_1B ((unsigned long)675*SMCLK_KHZ/WDT_CYCLES)

#define RESPONSE_MAX_TIME (7*(WDT_CYCLES/SMCLK_KHZ))

#define FLASH_DIVIDER     ((SMCLK_KHZ / 333)- 1)

/*  Reset Definitions              */
#define ACTUAL_LEVEL_RESET          254
#define POWER_ON_LEVEL_RESET        254
#define SYSTEM_FAILURE_LEVEL_RESET  254
#define MIN_LEVEL_RESET             PHYS_MIN_LEVEL
#define MAX_LEVEL_RESET             254
#define FADE_RATE_RESET             7
#define FADE_TIME_RESET             0
#define SEARCH_ADDRES_RESET         0xFFFFFF
#define RANDOM_ADDRES_RESET         0xFFFFFF
#define RANDOM_ADDR_BYTE_RESET      0xFF
#define GROUP_0_7_RESET             0x00
#define GROUP_8_F_RESET             0x00
#define SCENE_0_F_RESET             MASK
#define STATUS_INFO_CLR_RESET       0x98
#define STATUS_INFO_SET_RESET       0x20
#define DIMMING_CURVE_RESET         0

/*** Status Bits **************************************************************/
#define STATUS_OF_CNTL_GEAR         BIT0
#define LAMP_FAILURE                BIT1
#define LAMP_ARC_POWER_ON           BIT2
#define LIMIT_ERROR                 BIT3
#define FADE_RUNNING                BIT4
#define RESET_STATE                 BIT5
#define MISSING_SHORT_ADDRESS       BIT6 
#define STATUS_POWER_FAILURE        BIT7

/*** Response *****************************************************************/
#define YES                         0xFF

/*** Flash Offset definitions *************************************************/
#define POWER_ON_LEVEL              0
#define SYSTEM_FAILURE_LEVEL        1
#define  MIN_LEVEL                  2
#define  MAX_LEVEL                  3
#define  FADE_RATE                  4  
#define  FADE_TIME                  5
#define  SHORT_ADDRESS              6
#define  GROUP_0_7                  7
#define  GROUP_8_F                  8
#define  SCENE_0                    9
#define  SCENE_1                    10
#define  SCENE_2                    11
#define  SCENE_3                    12
#define  SCENE_4                    13
#define  SCENE_5                    14
#define  SCENE_6                    15
#define  SCENE_7                    16
#define  SCENE_8                    17
#define  SCENE_9                    18
#define  SCENE_A                    19
#define  SCENE_B                    20
#define  SCENE_C                    21
#define  SCENE_D                    22
#define  SCENE_E                    23
#define  SCENE_F                    24
#define RANDOM_ADDRESS_H            25
#define RANDOM_ADDRESS_M            26
#define RANDOM_ADDRESS_L            27
#define FAST_FADE_TIME              28
#define FAILURE_STATUS              29
#define OPERATING_MODE              30
#define DIMMING_CURVE               31

/*** Information Memory Defintions ********************************************/
extern const unsigned char Default_Constants_infoD[];
#define INFO_D1               ((unsigned char *)((&Default_Constants_infoD)))
#define INFO_D2               ((unsigned short) ((unsigned char *)(&Default_Constants_infoD))+0x20)
#define INFO_C1               ((unsigned short) ((unsigned char *)(&Default_Constants_infoD))+0x40)
#define INFO_C2               ((unsigned short) ((unsigned char *)(&Default_Constants_infoD))+0x60)
#define INFO_B1               ((unsigned short) ((unsigned char *)(&Default_Constants_infoD))+0x80)
#define INFO_B2               ((unsigned short) ((unsigned char *)(&Default_Constants_infoD))+0xA0)

#define LOWEST_INFO_SPACE     INFO_D1
#define HIGHEST_INFO_SPACE    INFO_B2

#define NUMBER_OF_SCENES      16
#define DATA_SIZE             32
#define SEGMENT_SIZE          64

/*** Default Definitions ******************************************************/
#define ACTUAL_LEVEL_DEFAULT          130
#define POWER_ON_LEVEL_DEFAULT        254
#define SYSTEM_FAILURE_LEVEL_DEFAULT  254
#define MIN_LEVEL_DEFAULT             PHYS_MIN_LEVEL
#define MAX_LEVEL_DEFAULT             254
#define FADE_RATE_DEFAULT             7
#define FADE_TIME_DEFAULT             0
#define SHORT_ADDRESS_DEFAULT         0x32
//#define SHORT_ADDRESS_DEFAULT         MASK
#define SEARCH_ADDRES_DEFAULT         0xFFFFFF
#define RANDOM_ADDRES_DEFAULT         0xFFFFFF
#define RANDOM_ADDR_BYTE_DEFAULT      0xFF
#define GROUP_0_7_DEFAULT             0x00
#define GROUP_8_F_DEFAULT             0x00
#define SCENE_0_F_DEFAULT             MASK
#define FAST_FADE_TIME_DEFAULT        0
#define STATUS_INFO_DEFAULT           0x80
#define FAILURE_STATUS_DEFAULT        0
#define OPERATING_MODE_DEFAULT        0
#define DIMMING_CURVE_DEFAULT         0   

/*** Memory Bank 0 Definitions ************************************************/
#define  BANK_0         0x00

#define  LAST_BNK0_ADDR 0x0E
#define  CHKSUM_BNK0    0x0A
#define  LAST_BNK       0x00
#define  GTIN_0         0xFF
#define  GTIN_1         0xFF
#define  GTIN_2         0xFF
#define  GTIN_3         0xFF   
#define  GTIN_4         0xFF
#define  GTIN_5         0xFF
#define  FRMWR_VER_0    0x00
#define  FRMWR_VER_1    0x00
#define  SERIAL_1       0xFF
#define  SERIAL_2       0xFF
#define  SERIAL_3       0xFF
#define  SERIAL_4       0xFF

#endif /*DALI_H_*/
