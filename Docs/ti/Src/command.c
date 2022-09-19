/******************************************************************************/
// command.c                                                                  
// Definition of TI_DALI_Command(), DALI commands 0-255                 
//                                                                            
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

#include "dali_demo_hw.h"
#include "dali.h"
#include "command.h"

/*** fade_200ms: fade up or down 200ms at the selected fade rate **************/
/* This number represents the number of to be added to or subtracted from the */
/* actual_level to achieve the new target_level.                              */ 
const unsigned char fade_200ms[16] = 
{ OFF, FADE_200ms_1, FADE_200ms_2, FADE_200ms_3, FADE_200ms_4, FADE_200ms_5,
  FADE_200ms_6, FADE_200ms_7, FADE_200ms_8, FADE_200ms_9, FADE_200ms_A,
  FADE_200ms_B, FADE_200ms_C, FADE_200ms_D, FADE_200ms_E, FADE_200ms_F
};

/*** fade_rate_table **********************************************************/
/* This array is used to set fade_step_size (in WDT intervals).  This         */
/* represents the number of times the WDT ISR is called before the arc power  */
/* level is updated. */ 
const unsigned int fade_rate_table[16] = 
{ OFF, FADE_RATE_1, FADE_RATE_2, FADE_RATE_3, FADE_RATE_4, FADE_RATE_5, 
  FADE_RATE_6, FADE_RATE_7, FADE_RATE_8, FADE_RATE_9, FADE_RATE_A, FADE_RATE_B,
  FADE_RATE_C, FADE_RATE_D, FADE_RATE_E, FADE_RATE_F
};

/*** fade_time_table **********************************************************/
/* This array represents the amount of time (in WDT intervals) that the       */
/* control gear will take to get from the actual_level to the target_level.   */
/* This array divided by the (target_level-acutal_level) provides the fade    */
/* rate which is loaded into fade_step_size.                                  */
const unsigned long int fade_time_table[16] = 
{ OFF, FADE_TIME_1, FADE_TIME_2, FADE_TIME_3, FADE_TIME_4, FADE_TIME_5,
  FADE_TIME_6, FADE_TIME_7, FADE_TIME_8, FADE_TIME_9, FADE_TIME_A, FADE_TIME_B,
  FADE_TIME_C, FADE_TIME_D, FADE_TIME_E, FADE_TIME_F
};

/*** fast_fade_time_table *****************************************************/
/* This fast fade time is to be used when fade time is 0.                     */
const unsigned long int fast_fade_time_table[28] = 
{ OFF, FAST_FADE_TIME_1, FAST_FADE_TIME_2, FAST_FADE_TIME_3, FAST_FADE_TIME_4,
  FAST_FADE_TIME_5, FAST_FADE_TIME_6, FAST_FADE_TIME_7, FAST_FADE_TIME_8,
  FAST_FADE_TIME_9, FAST_FADE_TIME_A, FAST_FADE_TIME_B, FAST_FADE_TIME_C,
  FAST_FADE_TIME_D, FAST_FADE_TIME_E, FAST_FADE_TIME_F, FAST_FADE_TIME_10,
  FAST_FADE_TIME_11, FAST_FADE_TIME_12, FAST_FADE_TIME_13, FAST_FADE_TIME_14,
  FAST_FADE_TIME_15, FAST_FADE_TIME_16, FAST_FADE_TIME_17, FAST_FADE_TIME_18,
  FAST_FADE_TIME_19, FAST_FADE_TIME_1A, FAST_FADE_TIME_1B
};

/******************************************************************************/
/*execute_command                                                             */
/******************************************************************************/
unsigned char TI_DALI_Command(unsigned char *rx_msg)
{ 
  unsigned char response=0;
  unsigned char update_value;
  if(rx_msg[0] & BIT0)  // Selector  bit
  { /* Command is found in rx_msg[1] Commands 0 through 255 */
    if((dapc_count < DAPC_TIMEOUT) && (rx_msg[1] != ENABLE_DAPC_SEQ))        
    {
      dapc_count = DAPC_TIMEOUT; // receiving another command stops the 
    }                              // sequence
    switch(rx_msg[1]) 
    {
      case TURN_OFF:
          IE1 &= ~WDTIE; /* Critical update */
          actual_level=OFF;
          status_information &= ~(FADE_RUNNING+LAMP_ARC_POWER_ON);
          status_information &= ~(LIMIT_ERROR+STATUS_POWER_FAILURE);
          IE1 |= WDTIE;
          TI_DALI_Update_Callback();
          break;
      case UP:
          if((actual_level != max_level) && 
              (status_information & LAMP_ARC_POWER_ON))
          {
            // Fade up for 200ms
            IE1 &= ~WDTIE; /* Critical update */
            fade_step_size = (unsigned long int)fade_rate_table[fade_rate];
            if(actual_level > (max_level - fade_200ms[fade_rate]))
            {
              target_level = max_level;
            }
            else
            { 
              target_level = actual_level + fade_200ms[fade_rate];
            }
            status_information |= FADE_RUNNING;
            IE1 |= WDTIE;
            break;
          }
          status_information |= LIMIT_ERROR;
          break;
      case DOWN:
          if((actual_level!=min_level) && 
              (status_information & LAMP_ARC_POWER_ON))
          {
            // Fade down for 200ms 
            IE1 &= ~WDTIE; /* Critical update */
            fade_step_size = (unsigned long int)fade_rate_table[fade_rate];
            if(actual_level < (min_level+fade_200ms[fade_rate]))
            { 
              target_level=min_level;
            }
            else
            {
              target_level = actual_level-fade_200ms[fade_rate];
            }
            status_information |= FADE_RUNNING;
            IE1 |= WDTIE;
            break;
          }
          status_information |= LIMIT_ERROR;
          break;
      case STEP_UP:
          if((actual_level!=max_level) && 
              (status_information & LAMP_ARC_POWER_ON))
          {
            IE1 &= ~WDTIE; /* Critical update */
            actual_level++;
            status_information &= ~FADE_RUNNING;
            IE1 |= WDTIE;
            TI_DALI_Update_Callback();
          }
          break;
      case STEP_DOWN:
          if((actual_level!=min_level) && 
              (status_information & LAMP_ARC_POWER_ON))
          {
            IE1 &= ~WDTIE; /* Critical update */
            actual_level--;
            status_information &= ~FADE_RUNNING;
            IE1 |= WDTIE;
            TI_DALI_Update_Callback();
          }
          break;
      case RECALL_MAX_LEVEL:
          IE1 &= ~WDTIE; /* Critical update */
          actual_level = max_level;
          status_information &= ~FADE_RUNNING;
          status_information |= LAMP_ARC_POWER_ON;
          status_information &= ~STATUS_POWER_FAILURE;
          IE1 |= WDTIE;            
          TI_DALI_Update_Callback();
          break;
      case RECALL_MIN_LEVEL:
          IE1 &= ~WDTIE; /* Critical update */
          actual_level = min_level;
          status_information &= ~FADE_RUNNING;
          status_information &= ~STATUS_POWER_FAILURE; 
          status_information |= LAMP_ARC_POWER_ON;
          IE1 |= WDTIE;
          TI_DALI_Update_Callback();                       
          break;
      case STEP_DOWN_AND_OFF:
          IE1 &= ~WDTIE; /* Critical update */
          status_information |= LIMIT_ERROR;
          if (actual_level > min_level)
          {
            actual_level--;
            status_information &= ~LIMIT_ERROR;
          }
          else 
          {// Turn off the PWMs
            actual_level=OFF;               
          }
          status_information &= ~FADE_RUNNING;
          status_information &= ~STATUS_POWER_FAILURE;
          IE1 |= WDTIE;
          TI_DALI_Update_Callback();                       
          break;
      case ON_AND_STEP_UP:
          IE1 &= ~WDTIE; /* Critical update */
          if (actual_level==OFF)
          { //Turn back on PWMs 
            actual_level = min_level;
          }
          status_information |= LIMIT_ERROR;
          if (actual_level < max_level)
          {
            actual_level++;
            status_information &= ~LIMIT_ERROR;
          }
          status_information &= ~FADE_RUNNING;
          status_information &= ~STATUS_POWER_FAILURE;
          status_information |= LAMP_ARC_POWER_ON;
          IE1 |= WDTIE;                       
          TI_DALI_Update_Callback();
          break;
#ifdef ENABLE_DAPC_SEQ
      case ENABLE_DAPC_SEQ:
          IE1 &= ~WDTIE; /* Critical update */
          dapc_count = 0;
          IE1 |= WDTIE;                                
          break;
#endif
      /*** Commands 16 through 31 are handled within the default case *********/

      /*** CONFIGURATION COMMANDS 32 -129 *************************************/
      /* These commands must be sent 2x within 100ms                          */
#ifdef RESET
      case RESET:
          if(config_count < CONFIG_TIMEOUT)
          {
            /* reset specified DALI variables */
            power_on_level = POWER_ON_LEVEL_RESET;
            system_failure_level = SYSTEM_FAILURE_LEVEL_RESET;
            min_level = MIN_LEVEL_RESET;
            max_level = MAX_LEVEL_RESET;
            fade_rate = FADE_RATE_RESET;
            fade_time = FADE_TIME_RESET;
            // short_address no change
            group_0_7 = GROUP_0_7_RESET;
            group_8_f = GROUP_8_F_RESET;
            random_address_byte_h = RANDOM_ADDR_BYTE_RESET;
            random_address_byte_m = RANDOM_ADDR_BYTE_RESET;
            random_address_byte_l = RANDOM_ADDR_BYTE_RESET;       
            actual_level = ACTUAL_LEVEL_RESET;
            search_address_byte_h = MASK;
            search_address_byte_m = MASK;
            search_address_byte_l = MASK;
            status_information &= ~STATUS_INFO_CLR_RESET;
            status_information |= STATUS_INFO_SET_RESET;          
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_ACTUAL_LEVEL_IN_THE_DTR
      case STORE_ACTUAL_LEVEL_IN_THE_DTR:
          if(config_count < CONFIG_TIMEOUT)
          {
            data_transfer_register = actual_level;
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_THE_DTR_AS_MAX_LEVEL
      case STORE_THE_DTR_AS_MAX_LEVEL:
          if(config_count < CONFIG_TIMEOUT)
          {
            if(data_transfer_register == MASK)
            {
              max_level = MASK-1;
            }
            else
            {
              max_level = data_transfer_register;
            }
            if(actual_level > max_level)
            {
              IE1 &= ~WDTIE; /* Critical update */
              actual_level = max_level;
              status_information &= ~FADE_RUNNING;
              IE1 |= WDTIE;
              TI_DALI_Update_Callback();                                 
            }
            flash_update_request=1;
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_THE_DTR_AS_MIN_LEVEL
      case STORE_THE_DTR_AS_MIN_LEVEL:  /* Command 43 */
          if(config_count < CONFIG_TIMEOUT)
          {
            if(data_transfer_register < PHYS_MIN_LEVEL)
            {
              min_level = PHYS_MIN_LEVEL;
            }
            else
            {
              min_level = data_transfer_register;
            }
            if((actual_level < min_level) && (actual_level != OFF))
            {
              IE1 &= ~WDTIE; /* Critical update */
              actual_level=min_level;
              status_information &= ~FADE_RUNNING; 
              IE1 |= WDTIE;
              TI_DALI_Update_Callback();                                
            }
            flash_update_request=1;
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL
      case STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL:  /* Command 44 */
          if(config_count < CONFIG_TIMEOUT)
          {
            system_failure_level = data_transfer_register;
            flash_update_request=1;
          }
          else
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_THE_DTR_AS_POWER_ON_LEVEL
      case STORE_THE_DTR_AS_POWER_ON_LEVEL:  /* Command 45 */
          if(config_count < CONFIG_TIMEOUT)
          {
            power_on_level = data_transfer_register;
            flash_update_request=1;
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_THE_DTR_AS_FADE_TIME
      case STORE_THE_DTR_AS_FADE_TIME:  /* Command 46 */
          if(config_count < CONFIG_TIMEOUT)
          {
            fade_time = data_transfer_register;
            flash_update_request=1;
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef STORE_THE_DTR_AS_FADE_RATE
      case STORE_THE_DTR_AS_FADE_RATE:  /* Command 47 */
          if(config_count < CONFIG_TIMEOUT)
          {
            fade_rate = data_transfer_register;
            flash_update_request=1;
          }
          else 
          {
            config_count=0;
          }         
          break;
      /*** Commands 48 through 63 are reserved ********************************/
      /*** Commands 64 through 127 are handled within the default case ********/
#endif
#ifdef STORE_THE_DTR_AS_SHORT_ADDRESS
      case STORE_THE_DTR_AS_SHORT_ADDRESS:  /* Command 128 */
          if(config_count < CONFIG_TIMEOUT)
          {
            short_address = data_transfer_register;
            flash_update_request=1;
          }
          else 
          {
            config_count=0;
          }         
          break;
#endif
#ifdef ENABLE_WRITE_MEMORY
      case ENABLE_WRITE_MEMORY:  /* Command 129 */
          /*** This command is not supported **********************************/
          break;
      /*** Commands 130 through 143 are reserved ******************************/
      /*** Query commands 144-223    */    
#endif
      case QUERY_STATUS:  /* Command 144 **************************************/
          response = status_information;
          break;   
      case QUERY_CONTROL_GEAR:  /* Command 145 */
          response = YES;
          break;   
      case QUERY_LAMP_FAILURE:  /* Command 146 */
          if(LAMP_FAILURE & status_information)
          {
            response = YES;
          }
          break;
      case QUERY_LAMP_POWER_ON:  /* Command 147 */
          if(LAMP_ARC_POWER_ON & status_information)
          {
            response = YES;
          }
          break;                                             
      case QUERY_LIMIT_ERROR:  /* Command 148 */
          if(LIMIT_ERROR & status_information)
          {
            response = YES;
          }
          break;
      case QUERY_RESET_STATE:  /* Command 149 */
          if(RESET_STATE & status_information)
          {
            response = YES;
          }
          break;                                                                                          
      case QUERY_MISSING_SHORT_ADDRESS:  /* Command 150 */
          if(MISSING_SHORT_ADDRESS & status_information)
          {
            response = YES;
          }
          break;
      case QUERY_VERSION_NUMBER:  /* Command 151 */
          response = version_number;
          break;
      case QUERY_CONTENT_DTR:  /* Command 152 */
          response = data_transfer_register;
          break;
      case QUERY_DEVICE_TYPE:  /* Command 153 */
          response = device_type;
          break;
      case QUERY_PHYSICAL_MINIMUM:  /* Command 154 */
          response = phys_min_lvl;
          break;
      case QUERY_POWER_FAILURE:  /* Command 155 */
          if(STATUS_POWER_FAILURE & status_information)
          {
            response = YES;
          }
          break;
      case QUERY_CONTENT_DTR1:  /* Command 156 */
          response = data_transfer_register1;
      case QUERY_CONTENT_DTR2:  /* Command 157 */
          response = data_transfer_register2;            
      /*** Commands 158 through 159 are reserved  *****************************/
      case QUERY_ACTUAL_LEVEL:  /* Command 160 */
          response = actual_level;
          break;
      case QUERY_MAX_LEVEL:  /* Command 161 */
          response = max_level;
          break;
      case QUERY_MIN_LEVEL:  /* Command 162 */
          response = min_level;
          break;
      case QUERY_POWER_ON_LEVEL:  /* Command 163 */
          response = power_on_level;
          break;
      case QUERY_SYSTEM_FAILUE_LEVEL:  /* Command 164 */
          response = system_failure_level;
          break;
      case QUERY_FADE_LEVEL:  /* Command 165 */
          response = fade_time;
          response = response << 4;
          response &= 0xF0;
          response |= fade_rate;
          break;
      /*** Commands 166 through 175 are reserved ******************************/
      /*** Commands 176 through 191 are Query Scene Level *********************/
      case QUERY_GROUPS_0_7:  /* Command 192 */
          response = group_0_7;
          break;
      case QUERY_GROUPS_8_F:  /* Command 193 */
          response = group_8_f;
          break;
      case QUERY_RANDOM_ADDR_H:  /* Command 194 */
          response = random_address_byte_h;
          break;
      case QUERY_RANDOM_ADDR_M:  /* Command 195 */
          response = random_address_byte_m;
          break;
      case QUERY_RANDOM_ADDR_L:  /* Command 196 */
          response = random_address_byte_l;
          break;         
      case READ_MEMORY_LOCATION: /* Command 197 */
          if(data_transfer_register1 == BANK_0)
          {
            /* Read from BANK 0 */
            if(data_transfer_register > LAST_BNK0_ADDR)
            {
              break;
            }
            response = memory_bank_0[data_transfer_register];
            data_transfer_register++;
            if(data_transfer_register > LAST_BNK0_ADDR)
            {
              break;
            }
            data_transfer_register2 = memory_bank_0[data_transfer_register]; 
          }
          break;
      /*** Commands 198 through 223 are reserved             */
      /*** Commands 224 through 227 are not supported        */
      case STORE_DTR_AS_FAST_FADE_TIME:  /* Command 228 */
          if((config_count < CONFIG_TIMEOUT) && enable_device_type)
          {
            if((data_transfer_register == 0) || 
                (data_transfer_register > min_fast_fade_time))
            {
              fast_fade_time = data_transfer_register;
            }
            else
            { 
              fast_fade_time = min_fast_fade_time;
            }
            enable_device_type = 0;
          }
          else 
          {
            config_count=0;
          }         
          break;
      /***  Commands 229 through 236 are reserved *****************************/
      case QUERY_GEAR_TYPE:  /* Command 237 */
          if(enable_device_type)
            {
              response = gear_type;
              enable_device_type = 0;
            }
          break;
      case QUERY_DIMMING_CURVE:  /* Command 238 */
          if(enable_device_type)
          {
            response = dimming_curve;
            enable_device_type = 0;
          }
          break;
      case QUERY_POSSIBLE_OPERATING_MODES:  /* Command 239 */
          if(enable_device_type)
          {
            response = possible_operating_modes;
            enable_device_type = 0;
          }
          break;
      case QUERY_FEATURES:  /* Command 240 */
          if(enable_device_type)
          {
            response = features;
            enable_device_type = 0;
          }
          break;
      case QUERY_FAILURE_STATUS:  /* Command 241 */
          if(enable_device_type)
          {
            response = failure_status;
            enable_device_type = 0;
          }
          break;
/*  Commands 242 through 251 are not supported        */
      case QUERY_OPERATING_MODE:  /* Command 252 */
          if(enable_device_type)
          {
            response = operating_mode;
            enable_device_type = 0;
          }
          break;
      case QUERY_FAST_FADE_TIME:  /* Command 253 */
          if(enable_device_type)
          {
            response = fast_fade_time;
            enable_device_type = 0;
          }
          break;
      case QUERY_MIN_FAST_FADE_TIME:  /* Command 254 */
          if(enable_device_type)
          {
            response = min_fast_fade_time;
            enable_device_type = 0;
          }
          break;  
      case QUERY_EXTENDED_VERSION_NUMBER:  /* Command 255 */
          if(enable_device_type)
          {
            response = extended_version_number;
            enable_device_type = 0;
          }
          break;  
      default:
          /*** Commands 16 through 31 are GO_TO_SCENE_X ***********************/
          if(((GO_TO_SCENE_0-1) < rx_msg[1]) &&
             (rx_msg[1] < (GO_TO_SCENE_F+1)))
          {
            if(scene[rx_msg[1]-GO_TO_SCENE_0] == MASK)
            {
              break;  // Do nothing
            }
            IE1 &= ~WDTIE; /* Critical Update */
            status_information &= ~STATUS_POWER_FAILURE;
            target_level = scene[rx_msg[1]-GO_TO_SCENE_0];
            if(target_level > max_level);
            {
              target_level = max_level;
            }                      
            if(target_level < min_level)
            {
              target_level = min_level;
            }
            /* fade to arc_level stored for scene */
            /* rx_msg[] is 16 through 31 */
            if((target_level == actual_level) || (fade_time == OFF))
            { 
              /* If already at the requested then result is nothing. */
              /* If no fade, then go to level immediately.           */
              status_information &= ~FADE_RUNNING;
              actual_level = scene[rx_msg[1]-GO_TO_SCENE_0];
              TI_DALI_Update_Callback();
            }
            else
            {
              /* Fade to level */
              status_information |= FADE_RUNNING;
              /* Calculate fade rate based upon fade time and change in power */
              /* level. */
              if(target_level > actual_level)
              {
                fade_step_size = fade_time_table[fade_time]/(target_level-actual_level);
              }
              else
              {
                fade_step_size = fade_time_table[fade_time]/(actual_level-target_level);
              }           
            }
            IE1 |= WDTIE; 
            break;          
          }          
          /*** Commands 64 through 79 are STORE THE DTR AS SCENE **************/
          if(((STORE_THE_DTR_AS_SCENE_0-1) < rx_msg[1]) &&
              (rx_msg[1] < (STORE_THE_DTR_AS_SCENE_F+1)))
          {
            if(config_count < CONFIG_TIMEOUT)
            { 
              scene[rx_msg[1]-STORE_THE_DTR_AS_SCENE_0]=data_transfer_register;
              flash_update_request=1;
            }
            else 
            {
              config_count=0;
            }
            break;
          }
          /*** Commands 80 through 95 are REMOVE FROM SCENE *******************/
          if(((REMOVE_FROM_SCENE_0-1) < rx_msg[1]) &&
             (rx_msg[1] < (REMOVE_FROM_SCENE_F+1)))
          {
            if(config_count < CONFIG_TIMEOUT)
            { 
              scene[rx_msg[1]-REMOVE_FROM_SCENE_0] = MASK;
              flash_update_request=1;
            }
            else 
            {
              config_count=0;
            }
            break;
          }     
          /*** Commands 96 through 111 are ADD TO GROUP ***********************/
          if(((ADD_TO_GROUP_0-1) < rx_msg[1]) &&
             ( rx_msg[1] < (ADD_TO_GROUP_8)))
          {
            if(config_count < CONFIG_TIMEOUT)
            { 
              update_value = 1;
              update_value = update_value<<(rx_msg[1]-ADD_TO_GROUP_0);
              group_0_7 |= update_value;
              flash_update_request=1;
            }
            else 
            {
              config_count=0;
            }
            break;
          }
          if(((ADD_TO_GROUP_7) < rx_msg[1]) &&
             (rx_msg[1] < (ADD_TO_GROUP_F+1)))
          {
            if(config_count < CONFIG_TIMEOUT)
            { 
              update_value = 1;
              update_value = update_value<<(rx_msg[1]-ADD_TO_GROUP_8);
              group_8_f |= update_value;
              flash_update_request=1;
            }
            else
            { 
              config_count=0;
            }
            break;
          }                                                                
          /*** Commands 112 through 127 are REMOVE FROM GROUP *****************/
          if(((REMOVE_FROM_GROUP_0-1) < rx_msg[1]) &&
             (rx_msg[1] < (REMOVE_FROM_GROUP_8)))
          {
            if(config_count < CONFIG_TIMEOUT)
            { 
              update_value = 1;
              update_value= update_value<<(rx_msg[1]-REMOVE_FROM_GROUP_0);
              update_value = ~update_value;
              group_0_7 &= update_value;
              flash_update_request=1;
            }
            else 
            {
              config_count=0;
            }
            break;
          }
          if(((REMOVE_FROM_GROUP_7) < rx_msg[1]) &&
             (rx_msg[1] < (REMOVE_FROM_GROUP_F+1)))
          {
            if(config_count < CONFIG_TIMEOUT)
            { 
              update_value = 1;
              update_value = update_value<<(rx_msg[1]-REMOVE_FROM_GROUP_8);
              update_value = ~update_value;
              group_8_f &= update_value;
              flash_update_request=1;
            }
            else 
            {
              config_count=0;
            }
            break;
          }
          /*** Commands 176 through 191 are Query Scene Level *****************/
          if(((QUERY_SCENE_LEVEL_0-1) < rx_msg[1]) &&
             (rx_msg[1] < (QUERY_SCENE_LEVEL_F+1)))
          {
            response = scene[rx_msg[1]-QUERY_SCENE_LEVEL_0];
            break;
          }
    } /* END CASE STATEMENT*/
  } /* END IF */
  /*** direct arc power level command found in rx_msg[1] **********************/
  else 
  {
    status_information &= ~STATUS_POWER_FAILURE;
    fade_count=0;       // restart counter
    /* If running at the fastest rate and not updated before next command */
    /* instantaneously go to target level.  IEC 62386_102 Section 9.5     */
    if((status_information & FADE_RUNNING) && (FADE_RATE == 1))
    {
      actual_level = target_level;
      status_information &= ~FADE_RUNNING;
    }
    if((rx_msg[1] == 0x00) || (rx_msg[1] == MASK)) /* fade to min and then off */
    {
      if(rx_msg[1])
      {
        status_information &= ~FADE_RUNNING;
        actual_level = target_level;
      }
      else
      {
        if(status_information & LAMP_ARC_POWER_ON)
        { /* If the lamp is on, then perform fade */
          IE1 &= ~WDTIE; /* Critical Update */
          status_information |= (FADE_RUNNING);
          status_information &= ~(LIMIT_ERROR);
          target_level=min_level-1; 
          /* Special case where target_level is less than min_level.  The fade  */
          /* handler in dali.c will recognize this condition and turn off after */
          /* the min_level has been reached.                                    */
          if(fade_time == OFF)
          {
            if(fast_fade_time != OFF)
            {
              fade_step_size = fast_fade_time_table[fast_fade_time];
              fade_step_size = fade_step_size/(actual_level-target_level);
            }
            else
            { 
              actual_level = min_level;
              fade_step_size = fade_time_table[fade_time];
              fade_step_size = fade_step_size/(actual_level-target_level);
            }
          }
        }
        else 
        {
          fade_step_size = fade_time_table[fade_time];
          fade_step_size = fade_step_size/(actual_level-target_level);
        }
        IE1 |= WDTIE; 
      }
    }
    else /* fade to level in rx_msg */
    {
      if(dapc_count < DAPC_TIMEOUT)
      {
        actual_level = target_level;
        dapc_count = 0;
      }
      IE1 &= ~WDTIE; /* Critical Update */
      if(!(status_information & LAMP_ARC_POWER_ON))
      { /* If the lamp is off, then turn on at minimum level */
        actual_level=min_level;
        status_information |= LAMP_ARC_POWER_ON;
        TI_DALI_Update_Callback();
      }
      /* limit value to min and max range */
      if(rx_msg[1] > max_level)
      {
        target_level = max_level;
        status_information |= LIMIT_ERROR;
      }
      else if(rx_msg[1] < min_level)
      {
        target_level = min_level;
        status_information |= LIMIT_ERROR;
      }
      else 
      {
        target_level = rx_msg[1];
        status_information &= ~(LIMIT_ERROR);
      }
      if((target_level == actual_level) || (fade_time == OFF))
      { 
          /* if already at the requested then result is nothing. */
          /* if no fade, then go to level immediately,           */
          status_information &= ~FADE_RUNNING;
          actual_level = target_level;
          TI_DALI_Update_Callback();
      }
      else
      {
        /* Fade to level */
        status_information |= FADE_RUNNING;
        /* Calculate fade rate based upon fade time and change in power level */
        if(target_level > actual_level)
        {
          fade_step_size = fade_time_table[fade_time];
          fade_step_size = fade_step_size/(target_level-actual_level);
        }
        else
        {
          fade_step_size = fade_time_table[fade_time];
          fade_step_size = fade_step_size/(actual_level-target_level);
        }
        if(dapc_count<DAPC_TIMEOUT)
        {
          fade_step_size = (unsigned long)DAPC_SEQ_SIZE;
        }
      }
      IE1 |= WDTIE;         
    }
  }/* End ELSE, Direct Arc Power level Command */
  return response;
}
