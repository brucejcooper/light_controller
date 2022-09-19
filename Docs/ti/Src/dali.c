/******************************************************************************/
// dali.c
// defines functions called by application as well as functions for the
// Manchester encoding phy and the address decoding
//
// Functions called by main application
//    void TI_DALI_Init(void)
//    void TI_DALI_Transaction_Loop(void)
//    void TI_DALI_Flash_Update(unsigned int)
//
// Functions called by TI_Dali_Transaction_Loop
//    void TI_DALI_Reset_Check(void)
//    void TI_DALI_Tx(unsigned char)
//    unsigned char TI_DALI_Rx(unsigned char *rx_msg)
//    unsigned char TI_DALI_Match_Address(unsigned char)
//    unsigned char TI_DALI_Command(unsigned char*)
//    unsigned char TI_DALI_Special_Command(unsigned char*)
//    void TI_DALI_Update_Callback(void);
//    void TI_DALI_Idle_Callback(void);
//
// ISR
// #pragma vector=WDT_VECTOR
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
#include "dali.h"
#include "dali_demo_hw.h"

/*** DALI Constants ***********************************************************/
const unsigned char min_fast_fade_time = 11;
const unsigned char gear_type = 0;
const unsigned char possible_operating_modes = 0;
const unsigned char features = 0;
const unsigned char version_number = 0;
const unsigned char extended_version_number=1;
const unsigned char device_type = 6;       /* Device type 6, device for LEDs */
const unsigned char phys_min_lvl = PHYS_MIN_LEVEL;
/*** Memory Bank 0 *************************************************************/
const unsigned char memory_bank_0[15] =
{
  LAST_BNK0_ADDR,
  CHKSUM_BNK0,
  LAST_BNK,
  GTIN_0,
  GTIN_1,
  GTIN_2,
  GTIN_3,
  GTIN_4,
  GTIN_5,
  FRMWR_VER_0,
  FRMWR_VER_1,
  SERIAL_1,
  SERIAL_2,
  SERIAL_3,
  SERIAL_4
};
/*** DALI Global RAM Variables ************************************************/
unsigned char power_on_level;
unsigned char actual_level;
/* SEARCH_ADDRESS and RANDOM_ADDRESS are defined in the DALI standard.  While */
// the strict definintion is not followed the funtionality is provided with
// three other variables (low, middle, and high) for each.
//unsigned long int search_address = SEARCH_ADDRES_DEFAULT;
//unsigned long int random_address = RANDOM_ADDRES_DEFAULT;
unsigned char search_address_byte_h;
unsigned char search_address_byte_m;
unsigned char search_address_byte_l;
unsigned char random_address_byte_h;
unsigned char random_address_byte_m;
unsigned char random_address_byte_l;
unsigned char system_failure_level;
unsigned char min_level;
unsigned char max_level;
unsigned char fade_rate;
unsigned char fade_time;
unsigned char fast_fade_time;
unsigned char short_address;
unsigned char group_0_7;
unsigned char group_8_f;
unsigned char scene[16];
unsigned char status_information;
unsigned char data_transfer_register;
unsigned char data_transfer_register1;
unsigned char data_transfer_register2;
unsigned char failure_status;
unsigned char operating_mode;
unsigned char dimming_curve;
/******************************************************************************/
// Additional Arc fade control global variables
unsigned char target_level;
unsigned long fade_count;
unsigned int config_count;
unsigned long dapc_count;       /*Direct Arc Power Control */
unsigned long start_time;
unsigned long fade_step_size;
unsigned char *flash_ptr;
unsigned char flash_update_request;
unsigned long initialise_count;
unsigned char initialise_minute;
unsigned char idle_time;
unsigned char physical_selection=0;
unsigned char enable_device_type=0;
/******************************************************************************/
// TI_Dali_Init(unsinged char)
//
/******************************************************************************/
void TI_DALI_Init(unsigned int key){
  unsigned char i;
  unsigned char *tmp_ptr;

  // GPIOs are already initialized in main using GPIO_INIT()
  // DALI_RX and DALI_TX pins should be initialized as GPIOS

  /*** Setup Memory and non-volatile variables ********************************/
  // Data exists in either INFO memory B,C, or D depending on what was last
  // written to the information memory space. After a factory program of flash
  // only INFOD will be programmed with the factory defaults.  The in system
  // flash routine in addition to this intitialization routine will provide a
  // cyclical buffer in the information memory. The valid range of the
  // MAX_LEVEL is min_level to 254 if MAX_LEVEL = 255 then that portion of the
  // segment is empty.
  flash_ptr = (unsigned char *)INFO_B2;
  tmp_ptr = (unsigned char *)INFO_D1;
  while((flash_ptr[MAX_LEVEL] == MASK))
  {
    flash_ptr = flash_ptr - DATA_SIZE;
    if((flash_ptr < (unsigned char *)LOWEST_INFO_SPACE))
    {
      while(1);    //trap cpu, device has lost even the factory default settings
    }
  }
  // There is a special case when the data was written to D1 from B2
  // (segment C is erased)
  if((flash_ptr == (unsigned char *)INFO_B2) && (tmp_ptr[MAX_LEVEL] != MASK))
  flash_ptr = tmp_ptr;
  power_on_level = flash_ptr[POWER_ON_LEVEL];

  /* Initialize RAM variables */
  system_failure_level = flash_ptr[SYSTEM_FAILURE_LEVEL];
  min_level = flash_ptr[MIN_LEVEL];
  max_level = flash_ptr[MAX_LEVEL];
  fade_rate = flash_ptr[FADE_RATE];
  fade_time = flash_ptr[FADE_TIME];
  short_address = flash_ptr[SHORT_ADDRESS];
  group_0_7 = flash_ptr[GROUP_0_7];
  group_8_f = flash_ptr[GROUP_8_F];
  for(i=0;i<NUMBER_OF_SCENES;i++)
  {
    scene[i] = flash_ptr[i+SCENE_0];
  }
  random_address_byte_h = flash_ptr[RANDOM_ADDRESS_H];
  random_address_byte_m = flash_ptr[RANDOM_ADDRESS_M];
  random_address_byte_h = flash_ptr[RANDOM_ADDRESS_L];
  fast_fade_time = flash_ptr[FAST_FADE_TIME];
  failure_status = (flash_ptr[FAILURE_STATUS] & 0x80);
  operating_mode = (flash_ptr[OPERATING_MODE] & 0x10);
  dimming_curve = flash_ptr[DIMMING_CURVE];
  search_address_byte_h = MASK;
  search_address_byte_m = MASK;
  search_address_byte_l = MASK;
  fade_count=0;
  flash_update_request = 0;
  config_count=CONFIG_TIMEOUT;
  dapc_count=DAPC_TIMEOUT;
  initialise_count=INITIALISE_SEC+1;
  status_information = STATUS_INFO_DEFAULT;
  /* Update the flash_ptr so that call to the TI_DALI_Flash_Update writes to  */
  /* the correct location in Information memory. */
  if(flash_ptr  == (unsigned char *)HIGHEST_INFO_SPACE)
  {
   flash_ptr = (unsigned char *)LOWEST_INFO_SPACE;
  }
  else
  {
   flash_ptr = flash_ptr+DATA_SIZE;
  }
  /* Erase the previous segement in preparation for the next power cycle. */
  FCTL2 = key+FSSEL1+FN0;
  FCTL3 = key;
  FCTL1 = key+ERASE;
  if((flash_ptr == (unsigned char *)INFO_D2))
  {
  tmp_ptr = (unsigned char *)INFO_B1;
  tmp_ptr[POWER_ON_LEVEL] = 0; /* Dummy write segment B */
  }
  if((flash_ptr == (unsigned char *)INFO_C2))
  {
  tmp_ptr = (unsigned char *)INFO_D1;
  tmp_ptr[POWER_ON_LEVEL] = 0; /* Dummy write segment D */
  }
  if((flash_ptr == (unsigned char *)INFO_B2))
  {
  tmp_ptr = (unsigned char *)INFO_C1;
  tmp_ptr[POWER_ON_LEVEL] = 0; /* Dummy write segment C */
  }
  FCTL1 = key;
  FCTL3 = key+LOCK;
  key=0;

  /*** Setup WDT **************************************************************/
  WDTCTL = FADE_INTERVAL;
  IE1 |= WDTIE;
  actual_level = power_on_level;
  if(power_on_level<min_level)
  {
    actual_level = min_level;
  }
  if(power_on_level>max_level)
  {
    actual_level = max_level;
  }
  start_time = 0;  // Initialize timer.  If expires go to power on level.
} // END TI_Dali_Init()

/******************************************************************************/
// TI_Dali_Transaction_Loop
// This function gets the DALI received data, rx_msg[], calls the appropriate
// decode function, execute_command or execute_special_command, and then calls
// back to the main application. After returning from the main application a
// response transmission is sent if needed and then repeats.
/******************************************************************************/
void TI_DALI_Transaction_Loop(void)
{
  /* the information pushed onto the stack for this call is not needed */
  /* possibly adjust stact pointer?                                    */
  unsigned char bytes_received, command_type;
  static unsigned char rx_msg[VALID_BYTES];
  static unsigned char response;
  while (1)
  {
    response=0;
    bytes_received=0;
    command_type = 0;
    bytes_received = TI_DALI_Rx(rx_msg);
    if(bytes_received == SYSTEM_FAILURE)
      {
        status_information |= STATUS_POWER_FAILURE;
        status_information &= ~FADE_RUNNING;
        //arc_power_level(system_failure_level);
        /* check system_failure_level, if 255 (mask) then no change
        *  if a different value then go to that value without fading
        */
        if (system_failure_level != MASK)
        {
          /* Update both status and actual_level before WDT call*/
          IE1 &= ~WDTIE;
          actual_level = system_failure_level;
          if(system_failure_level > max_level)
          {
            actual_level = max_level;
          }
          if(system_failure_level < min_level)
          {
            actual_level = min_level;
          }
          status_information |= LAMP_ARC_POWER_ON;
          status_information &= ~FADE_RUNNING;
          IE1 |= WDTIE;
          TI_DALI_Update_Callback();
        }
      }
      else if (bytes_received == VALID_BYTES)
      {
        /*Decode address*/
        command_type = TI_DALI_Match_Address(rx_msg[0]);
        // if address is a match return a 1 or 2
        if(command_type == 1)
        {
          response = TI_DALI_Command(rx_msg);
        }
        if(command_type == 2)
        {
          response = TI_DALI_Special_Command(rx_msg);
        }
        if(response)
        {
          /* Respond, if the response is 'NO', then control gear does not     */
          /* react. The response must occur between 7 and 22 TE after the     */
          /* forward frame. 7 TE ~ 3ms , 22*TE ~ 9ms                          */
          /*  Use the WDT to determine the amount of time that must be waited */
          /*  after the return from the callback function.                    */
          idle_time = 0;
          TI_DALI_Idle_Callback();
          while(idle_time < RESPONSE_MAX_TIME);
          TI_DALI_Tx(response);
        }
        else
        {
          TI_DALI_Idle_Callback();
        }
        TI_DALI_Reset_Check(); // if the varaibles are no longer consistent with RESET
                       // then clear the bit from the status register
      }//ignore command if does not meet length criteria
  }// END WHILE(1)
}

/******************************************************************************/
// TI_DALI_Rx
// This function stores the received data into the array rx_msg[] and returns
// the number of bytes receieved.  The return value is also used to report if
// a system failure or timing violation has occured.
/******************************************************************************/
unsigned char TI_DALI_Rx(unsigned char* rx_msg)
{
  unsigned long fault_check=0,stop_check=0;
  unsigned char rx_bytes,rx_bits,temp_msg;
  unsigned int rx_mask;
  rx_bytes=rx_bits=0;

  if(!(DALI_RX_PxIN & DALI_RX_BIT))
  {
    DALI_RX_PxIES &= ~DALI_RX_BIT;       // Low-to-High Transition
    DALI_RX_PxIFG &= ~DALI_RX_BIT;
    fault_check=0;
    while (~DALI_RX_PxIFG & DALI_RX_BIT)     // Wait for rising edge
    {
      if (fault_check++ > 500*MCLK_mSEC_COUNT) // Interface failure when > 500ms
      {
        return SYSTEM_FAILURE;
      }
    }
  }
  DALI_RX_PxIES |= DALI_RX_BIT;        // High-to-Low Transition
  DALI_RX_PxIFG &= ~DALI_RX_BIT;
  /*** START BIT ****/
  while(~DALI_RX_PxIFG & DALI_RX_BIT);   // Wait for Falling Edge
  DALI_RX_PxIES &= ~DALI_RX_BIT;       // Low-to-High Transition
  DALI_RX_PxIFG &= ~DALI_RX_BIT;
  /****************************************************************************/
  // Fault detection: if the input is low for greater than 500ms, then the
  // light should go to the fault setting.
  /****************************************************************************/
  fault_check=0;
  while (~DALI_RX_PxIFG & DALI_RX_BIT)     // Wait for rising edge
  {
    if (fault_check++ > 505*MCLK_uSEC_COUNT) // bit failure if greater than
    {                                        // tmax = 500us
      return 0;
    }
  }
  /* Start pulse is special case, receive first bit */
  __delay_cycles(505*MCLK_uSEC_COUNT); // maximum width is 500us
  rx_msg[rx_bytes] = 0;
  rx_mask = (DALI_RX_PxIN & DALI_RX_BIT) ^ DALI_RX_BIT;
  rx_msg[rx_bytes] |= (rx_mask<<7);
  rx_bits++;
  /* wait for edge */
  fault_check=0;
  if (rx_mask)
  {
      DALI_RX_PxIES &= ~DALI_RX_BIT;       // Low-to-High Transition
  }
  else
  {
      DALI_RX_PxIES |= DALI_RX_BIT;        // High-to-Low Transition
  }
  DALI_RX_PxIFG &= ~DALI_RX_BIT;
  while (~DALI_RX_PxIFG & DALI_RX_BIT)  // wait for change in state
  {
    if (fault_check++ > 505*MCLK_uSEC_COUNT) // Interface failure when > 500ms
    {
      return 0;                           // timing fault
    }
  }
  while (rx_bytes<VALID_BYTES)
  {
    while (rx_bits <8)
    {
      __delay_cycles(505*MCLK_uSEC_COUNT);
      // maximum TE is 500us + 5us for extra hold time
      rx_mask = (DALI_RX_PxIN & DALI_RX_BIT) ^ DALI_RX_BIT;
      fault_check=0;
      rx_mask = rx_mask<<(7-rx_bits);
      rx_msg[rx_bytes] |= rx_mask;
      rx_bits++;
      if (rx_mask)
      {
          DALI_RX_PxIES &= ~DALI_RX_BIT;       // Low-to-High Transition
      }
      else
      {
          DALI_RX_PxIES |= DALI_RX_BIT;        // High-to-Low Transition
      }
      DALI_RX_PxIFG &= ~DALI_RX_BIT;
      while (~DALI_RX_PxIFG & DALI_RX_BIT)  // wait for change in state
      {
        if (fault_check++ > 505*MCLK_uSEC_COUNT)
        {
          return 0;
        }
      }
    } // End rx_bits while loop
    /* Sample possible first bit of next byte */
    rx_bytes++;
    temp_msg = 0;
    rx_bits=0;
    stop_check=0;
    // maximum TE is 500us + 5us for extra hold time
    __delay_cycles(505*MCLK_uSEC_COUNT);
    rx_mask = (DALI_RX_PxIN & DALI_RX_BIT) ^ DALI_RX_BIT;
    temp_msg |= rx_mask<<7;
    rx_bits++;
    /* wait for edge */
    if (rx_mask)
    {
        DALI_RX_PxIES &= ~DALI_RX_BIT;       // Low-to-High Transition
    }
    else
    {
        DALI_RX_PxIES |= DALI_RX_BIT;        // High-to-Low Transition
    }
    DALI_RX_PxIFG &= ~DALI_RX_BIT;
    while (~DALI_RX_PxIFG & DALI_RX_BIT)  // wait for change in state
    {
      /*check to see if no edges have occured, then this is a stop condition*/
      /* Stop bits are 4*TE or 4*416.67us = 1667                            */
      /* x cycles per while loop                                            */
      if (stop_check > 50*MCLK_uSEC_COUNT)
      {
        return rx_bytes;
      }
      stop_check++;
    }
      if(rx_bytes >= VALID_BYTES)
      {
        return 0;
      }
      else
      {
        rx_msg[rx_bytes] = temp_msg;
      }
  } // End rx_bytes Whileloop
  return 0;
}

/******************************************************************************/
// TI_DALI_Tx(response)
// This function transmits the query response. The response is requested by
// the command query.
// P2.1 is TX line
// Start bit, data byte, 2 stop bits (idle line)
/******************************************************************************/

void TI_DALI_Tx(unsigned char response)
{
  unsigned char bit_mask=0x80;
  /* Start Bit  */
  DALI_TX_PxOUT &= ~DALI_TX_BIT;
  __delay_cycles(416*MCLK_uSEC_COUNT);
  DALI_TX_PxOUT |= DALI_TX_BIT;
  __delay_cycles(416*MCLK_uSEC_COUNT);
  while(bit_mask)
  {
    if(bit_mask & response)
    { /* transmit a '1' */
        DALI_TX_PxOUT &= ~DALI_TX_BIT;
      __delay_cycles(416*MCLK_uSEC_COUNT);
      DALI_TX_PxOUT |= DALI_TX_BIT;
      __delay_cycles(416*MCLK_uSEC_COUNT);
    }
    else
    { /* transmit a '0' */
      DALI_TX_PxOUT |= DALI_TX_BIT;
      __delay_cycles(416*MCLK_uSEC_COUNT);
      DALI_TX_PxOUT &= ~DALI_TX_BIT;
      __delay_cycles(416*MCLK_uSEC_COUNT);
    }
    bit_mask = bit_mask >> 1;
  }
  DALI_TX_PxOUT |= DALI_TX_BIT; /* IDLE condition */
}

/******************************************************************************/
// TI_DALI_Match_Address()
//
// Is the address field a Broadcast, group, or short address
// rx_msg[0]?= broadcast (b)
//          ?= group     (g)
//          ?= short     (s)
// Return a 1 if it is a match with either a b,g, or s
// return a 2 if it is a special command
/******************************************************************************/
unsigned char TI_DALI_Match_Address(unsigned char address){
  unsigned char temp;
  if((address > 0x9F) && (address < 0xFE))
  {
    /* Special commands are address 0xB1 through 0xC7         */
    /* while valid address values range from 0x00 to 0x7F     */
    /* group address values range from 0x80 to 0x9F           */
    /* and the broadcast address values are 0xFE and 0xFF     */
    return 2;
  }
  if(address & BIT7)
  {/* group or broadcast */
    address = address>>1;
    address = address & 0x7F;
    if(address==0x7F)
    {
      return 1; /* broadcast message */
    }
    address = address & 0x0F;  /* group id is now in lower nibble */
    temp = 0x01;
    if(address<8)
    {
      temp = temp<<address;
      if(temp & group_0_7)
      {
        return 1;
      }
    }
    else
    {
      address = address - 8;
      temp = temp<<address;
      if(temp & group_8_f)
      {
        return 1;
      }
    }
  }
  address = address>>1;
  address = address & 0x3F;
  if(address == short_address)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/******************************************************************************/
// TI_DALI_Reset_Check()
// If the values in the information register match the RESET value
// , then the RESET bit must be set
/******************************************************************************/
void TI_DALI_Reset_Check(void)
{
  if(actual_level != ACTUAL_LEVEL_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(system_failure_level != SYSTEM_FAILURE_LEVEL_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(power_on_level != POWER_ON_LEVEL_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(min_level != MIN_LEVEL_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(max_level != MAX_LEVEL_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(fade_rate != FADE_RATE_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(fade_time != FADE_TIME_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(fade_time != FADE_TIME_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(group_0_7 != GROUP_0_7_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(group_8_f != GROUP_8_F_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(scene[0] != SCENE_0_F_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  if(dimming_curve != DIMMING_CURVE_RESET)
  {
    status_information &= ~RESET_STATE;
    return;
  }
  status_information |= RESET_STATE;
}


/******************************************************************************/
// TI_Flash_Update(FWKEY)
// Use the FLASH Controller to save these non-volatile settings. These
// settings will be used on the next Power up.
/******************************************************************************/
void TI_DALI_Flash_Update(unsigned int key)
{
  /* move data to next location */
  unsigned char i;

  IE1 &= ~WDTIE;
  WDTCTL = WDTPW+WDTHOLD;  // stop the WDT
  FCTL2 = key+FSSEL0+FLASH_DIVIDER; // 333kHz Flash Timing Generator
  FCTL3 = key;
  FCTL1 = key+WRT;
  flash_ptr[POWER_ON_LEVEL] = power_on_level;
  flash_ptr[SYSTEM_FAILURE_LEVEL] = system_failure_level;
  flash_ptr[MIN_LEVEL] = min_level;
  flash_ptr[MAX_LEVEL] = max_level;
  flash_ptr[FADE_RATE] = fade_rate;
  flash_ptr[FADE_TIME] = fade_time;
  flash_ptr[SHORT_ADDRESS] = short_address;
  flash_ptr[GROUP_0_7] = group_0_7;
  flash_ptr[GROUP_8_F] = group_8_f;
  for(i=0;i<NUMBER_OF_SCENES;i++)
  {
    flash_ptr[i+SCENE_0]= scene[i];
  }
  flash_ptr[RANDOM_ADDRESS_H] = random_address_byte_h;
  flash_ptr[RANDOM_ADDRESS_M] = random_address_byte_m;
  flash_ptr[RANDOM_ADDRESS_L] = random_address_byte_l;
  flash_ptr[FAST_FADE_TIME] = fast_fade_time;
  flash_ptr[FAILURE_STATUS] = failure_status;
  flash_ptr[OPERATING_MODE] = operating_mode;
  flash_ptr[DIMMING_CURVE] = dimming_curve;
  /* scene o through 15 are set to FF which is the value of erased flash */
  FCTL1 = key;  /* lock flash */
  FCTL3 = key+LOCK;
  /* update the global pointer */
}

/******************************************************************************/
// Watch Dog Timer in Interval Mode
// The DALI specficiation requires that the device fade to the requested power
// level.  The fade rate can be one of 14 values defined in the DALI standard.
// This function uses the periodic interval of the WDT to determine if the
// power level needs to be updates because the controller is fading to a
// requested level.
// Timer is also used to evaluate four different time delays:
// Direct Arc Power Sequence (200ms)
// Configuration (100ms)
// Initialisation (15 minutes)
// Response idle time (7ms)
/******************************************************************************/
#pragma vector=WDT_VECTOR
__interrupt void ISR_WDT(void)
{
  if(status_information & FADE_RUNNING)
  {
    if(fade_count >= fade_step_size)
    {
      fade_count=0;
      if(target_level > actual_level)
      {
        actual_level++;
      }
      if(target_level < actual_level)
      {
        actual_level--;
      }
      if(actual_level == target_level)
      {
        status_information &= ~FADE_RUNNING;
        if(target_level == (min_level-1))
        {                         // special case of fade to min and then off
          actual_level=OFF;
          status_information &= ~LAMP_ARC_POWER_ON;
        }
      }
      TI_DALI_Update_Callback();
    }
    else fade_count++;
  }
  /* If the control gear does not receive a power level command within 0.6 */
  /* seconds, then the control shall go to the power_on_level immediately */
  if(start_time<POWER_ON_TIME)
  {
    start_time++;
    if((start_time == POWER_ON_TIME) && (actual_level == power_on_level))
    {
      TI_DALI_Update_Callback();
    }
  }
  /* The control gear must respone within 7 to 22 TE or 3 to 9 ms */
  if(idle_time<RESPONSE_MAX_TIME)
  {
    idle_time++;
  }
  /* Configuration commands must be duplicated */
  /* within 100ms or they are ignored */
  if(config_count<CONFIG_TIMEOUT)
  {
    config_count++;
  }
  /* DAPC sequence  */
  if(dapc_count<DAPC_TIMEOUT)
  {
    dapc_count++;
  }
  /* Initialise sequence 15 minutes or 15*60*1000 = 15*60000 */
  if(initialise_count<INITIALISE_SEC)
  {
    initialise_count++;
  }
  else
  {
    initialise_count=0;
    initialise_minute++;
    if(initialise_minute == INITIALISE_MIN)
    {
      initialise_count=INITIALISE_SEC;
      initialise_minute = 0;
    }
  }
} // END WDT ISR
