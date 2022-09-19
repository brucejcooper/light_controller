/******************************************************************************/
// special_command.c
// Defines TI_DALI_Special_Command()  
// Special commands are commands 256 through 271, extended special commands 
// have the same structure and are commands 272 - 275, 276-383 are reserved
// for future use.                                                         
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

#include "dali_demo_hw.h"
#include "dali.h"
#include "special_command.h"

unsigned char TI_DALI_Special_Command(unsigned char *rx_msg)
{ 
/* Commands 256 through 275  */
  unsigned char temp_address;
  unsigned char response=0;
  switch(rx_msg[0]) 
  {    
#ifdef TERMINATE                
    case TERMINATE: /* Command 256  */
        initialise_count=INITIALISE_SEC;
        break;
#endif
#ifdef DATA_TRANSFER_REGISTER                        
    case DATA_TRANSFER_REGISTER:/* Command 257  */
        data_transfer_register = rx_msg[1];
        break;
#endif
#ifdef INITIALISE
    /* A 15 minute timer is started with Initialise        */
    /* During this time commands 259-270 can be processed  */            
    case INITIALISE:            /*  Commands 258 */
        /* rx_msg[1] = 0 : broadcast 
        *            = 0xFF : all devices without a short address
        *            = 0AAA AAA1 : device with address AA AAAA  */
        temp_address = rx_msg[1]>>1;
        temp_address &= ~(BIT7+BIT6);       //
        if((rx_msg[1] == 0))
        { 
            temp_address = short_address;
        }
        if((rx_msg[1] == MASK) && (status_information & MISSING_SHORT_ADDRESS))
        { 
            temp_address = short_address;
        }
        if (temp_address == short_address)
        {
          if(config_count < CONFIG_TIMEOUT)
          { /* need to receive the command twice */
            initialise_count=0;
            initialise_minute=0;
          }
          else
          {
            config_count=0;
          }
        }
        break;
#endif            
#ifdef RANDOMISE            
    case RANDOMISE:            /*  Commands 259 */
        if(initialise_count==INITIALISE_SEC)
        {
          break;
        }  
        if(config_count < CONFIG_TIMEOUT)
        { /* need to receive the command twice */
          /* generate random address  0 - FFFFFF*/
          if(random_address_byte_m == CALDCO_1MHZ)
          {
            random_address_byte_m = CALDCO_12MHZ;
          }
          else 
          {
            random_address_byte_m = CALDCO_1MHZ;
          }
          if(random_address_byte_h == CALDCO_1MHZ)
          {
            random_address_byte_h = CALDCO_12MHZ;
          }
          else 
          {
            random_address_byte_h = CALDCO_1MHZ;
          }
          flash_update_request=1;
        }
        else 
        {
          config_count=0;
        }
        break;
#endif
#ifdef COMPARE            
    case COMPARE:            /*  Commands 260 */
        if(initialise_count==INITIALISE_SEC)
        {
          break;  
        }
        if(random_address_byte_h > search_address_byte_h)
        {
          break;
        }
        if(random_address_byte_h == search_address_byte_h)
        {
          if(random_address_byte_m > search_address_byte_m)
          {
            break;
          }      
          if(random_address_byte_m == search_address_byte_m)
          {
            if(random_address_byte_l > search_address_byte_l)
            {
              break;
            }                  
          }           
        }
        response = YES;
        break;
#endif
#ifdef WITHDRAW            
    case WITHDRAW:            /*  Commands 261 */
        /* exclude from compare process if random_address = search_address */
        initialise_count=INITIALISE_SEC;
        break;
#endif
/* Commands 262 and 263 are reserved for future use  */
#ifdef SEARCHADDRH            
    case SEARCHADDRH:            /*  Commands 264 */
        if(initialise_count<INITIALISE_SEC)
        {
          search_address_byte_h = rx_msg[1];
        }  
        break;
#endif
#ifdef SEARCHADDRM            
    case SEARCHADDRM:            /*  Commands 265 */
        if(initialise_count<INITIALISE_SEC)
        {
          search_address_byte_m = rx_msg[1];
        }  
        break;
#endif
#ifdef SEARCHADDRL            
    case SEARCHADDRL:            /*  Commands 266 */
        if(initialise_count<INITIALISE_SEC)
        {
          search_address_byte_l = rx_msg[1];
        }  
        break;
#endif
#ifdef PROGRAM_SHORT_ADDRESS
    case PROGRAM_SHORT_ADDRESS:  /* Command 267  */
        if(initialise_count==INITIALISE_SEC)
        {
          break;
        }
        if(search_address_byte_h == random_address_byte_h)
        {
          if(search_address_byte_m == random_address_byte_m)
          {
            if(search_address_byte_l == random_address_byte_l)
            {
              physical_selection = 1;
            }
          }
        }
        if(physical_selection)
        {
          if(rx_msg[1] & BIT7)
          {
            /* Delete short address */
            short_address = MASK;
            status_information |= MISSING_SHORT_ADDRESS;
          }
          else
          {
            short_address = rx_msg[1]>>1;        // Address  xAAAAAA1
            short_address &= ~(BIT7+BIT6);       //
          }
          flash_update_request=1;
        }
        break;
#endif                      
    case VERIFY_SHORT_ADDRESS: /* Command 268  */
        if(initialise_count==INITIALISE_SEC)
        {
          break;
        }  
        temp_address = rx_msg[1]>>1;
        temp_address &= ~(BIT7+BIT6);       
        if (temp_address == short_address)
        {
          response = YES;
        } 
        break;
    case QUERY_SHORT_ADDRESS: /* Command 269  */
        if(initialise_count==INITIALISE_SEC)
        {
          break;
        }
        if(search_address_byte_h == random_address_byte_h)
        {
          if(search_address_byte_m == random_address_byte_m)
          {
            if(search_address_byte_l == random_address_byte_l)
            {
              physical_selection = 1;
            }
          }
        }
        if(physical_selection)
        {
          response = short_address;
        } 
        break;
    case PHYSICAL_SELECTION: /* Command 270  */
        if(initialise_count==INITIALISE_SEC)
        {
          break;
        }
        physical_selection = 1;
        break;
    case ENABLE_DEVICE_TYPE_X: /* Command 272  */
        if(rx_msg[1] == device_type)
        {
          enable_device_type = 1;
        }
        break;
#ifdef DATA_TRANSFER_REGISTER_1                        
    case DATA_TRANSFER_REGISTER_1:/* Command 273  */
        data_transfer_register1 = rx_msg[1];
        break;
#endif
#ifdef DATA_TRANSFER_REGISTER_2                        
    case DATA_TRANSFER_REGISTER_2:/* Command 274  */
        data_transfer_register2 = rx_msg[1];
        break;
#endif
    default:
        break;              
  } //END SWITCH
  return response;
}
