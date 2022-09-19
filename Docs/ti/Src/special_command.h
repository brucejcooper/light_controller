/******************************************************************************/
// special_command.h                                                          
// definitions associated with special DALI commands                                            
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
#ifndef SPECIAL_COMMAND_H_
#define SPECIAL_COMMAND_H_

/*** function prototypes ******************************************************/
unsigned char TI_DALI_Special_Command(unsigned char*);

/*** Special Commands *********************************************************/
#define TERMINATE					      0xA1
#define DATA_TRANSFER_REGISTER  0xA3
#define INITIALISE					    0xA5
#define RANDOMISE					      0xA7
#define COMPARE						      0xA9
#define WITHDRAW					      0xAB
/* 262-263 are reserved  */
#define SEARCHADDRH					    0xB1
#define SEARCHADDRM					    0xB3
#define SEARCHADDRL					    0xB5
#define PROGRAM_SHORT_ADDRESS		0xB7
#define VERIFY_SHORT_ADDRESS		0xB9
#define QUERY_SHORT_ADDRESS			0xBB
#define PHYSICAL_SELECTION			0xBD

#define ENABLE_DEVICE_TYPE_X		  0xC1
#define DATA_TRANSFER_REGISTER_1	0xC3
#define DATA_TRANSFER_REGISTER_2	0xC5

#endif /*SPECIAL_COMMAND_H_*/
