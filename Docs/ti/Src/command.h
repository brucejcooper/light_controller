/******************************************************************************/
// command.h                                                                  
// definitions for DALI and clock                                             
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

#ifndef COMMAND_H_
#define COMMAND_H_

/* Function Prototypes  */
unsigned char TI_DALI_Command(unsigned char*);

/* Fade time greater than or equal to 200ms */
#define DAPC_SEQ_SIZE   200*SMCLK_KHZ/WDT_CYCLES

/* Indirect arc power control commands */
#define TURN_OFF            0x00
#define UP                  0x01
#define DOWN                0x02
#define STEP_UP             0x03
#define STEP_DOWN           0x04
#define RECALL_MAX_LEVEL    0x05
#define RECALL_MIN_LEVEL    0x06
#define STEP_DOWN_AND_OFF   0x07
#define ON_AND_STEP_UP      0x08
#define ENABLE_DAPC_SEQ     0x09
/* Commands 0x0A-0x0F are reserved for future use */
#define GO_TO_SCENE_0   0x10
#define GO_TO_SCENE_1   0x11
#define GO_TO_SCENE_2   0x12
#define GO_TO_SCENE_3   0x13
#define GO_TO_SCENE_4   0x14
#define GO_TO_SCENE_5   0x15
#define GO_TO_SCENE_6   0x16
#define GO_TO_SCENE_7   0x17
#define GO_TO_SCENE_8   0x18
#define GO_TO_SCENE_9   0x19
#define GO_TO_SCENE_A   0x1A
#define GO_TO_SCENE_B   0x1B
#define GO_TO_SCENE_C   0x1C
#define GO_TO_SCENE_D   0x1D
#define GO_TO_SCENE_E   0x1E
#define GO_TO_SCENE_F   0x1F

/* Configuration Commands 0x20-81            */
#define RESET               0x20
#define STORE_ACTUAL_LEVEL_IN_THE_DTR 0x21

/* Commands 0x22-0x29 are reserved  */

/* Arc Power parameter settings */
#define STORE_THE_DTR_AS_MAX_LEVEL  0x2A
#define STORE_THE_DTR_AS_MIN_LEVEL  0x2B
#define STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL 0x2C
#define STORE_THE_DTR_AS_POWER_ON_LEVEL   0x2D
#define STORE_THE_DTR_AS_FADE_TIME  0x2E
#define STORE_THE_DTR_AS_FADE_RATE  0x2F

/* Commands 48-63 are reserved */

#define STORE_THE_DTR_AS_SCENE_0  0x40
#define STORE_THE_DTR_AS_SCENE_1  0x41
#define STORE_THE_DTR_AS_SCENE_2  0x42
#define STORE_THE_DTR_AS_SCENE_3  0x43
#define STORE_THE_DTR_AS_SCENE_4  0x44
#define STORE_THE_DTR_AS_SCENE_5  0x45
#define STORE_THE_DTR_AS_SCENE_6  0x46
#define STORE_THE_DTR_AS_SCENE_7  0x47
#define STORE_THE_DTR_AS_SCENE_8  0x48
#define STORE_THE_DTR_AS_SCENE_9  0x49
#define STORE_THE_DTR_AS_SCENE_A  0x4A
#define STORE_THE_DTR_AS_SCENE_B  0x4B
#define STORE_THE_DTR_AS_SCENE_C  0x4C
#define STORE_THE_DTR_AS_SCENE_D  0x4D
#define STORE_THE_DTR_AS_SCENE_E  0x4E
#define STORE_THE_DTR_AS_SCENE_F  0x4F

#define REMOVE_FROM_SCENE_0   0x50
#define REMOVE_FROM_SCENE_1   0x51
#define REMOVE_FROM_SCENE_2   0x52
#define REMOVE_FROM_SCENE_3   0x53
#define REMOVE_FROM_SCENE_4   0x54
#define REMOVE_FROM_SCENE_5   0x55
#define REMOVE_FROM_SCENE_6   0x56
#define REMOVE_FROM_SCENE_7   0x57
#define REMOVE_FROM_SCENE_8   0x58
#define REMOVE_FROM_SCENE_9   0x59
#define REMOVE_FROM_SCENE_A   0x5A
#define REMOVE_FROM_SCENE_B   0x5B
#define REMOVE_FROM_SCENE_C   0x5C
#define REMOVE_FROM_SCENE_D   0x5D
#define REMOVE_FROM_SCENE_E   0x5E
#define REMOVE_FROM_SCENE_F   0x5F

#define ADD_TO_GROUP_0      0x60
#define ADD_TO_GROUP_1      0x61
#define ADD_TO_GROUP_2      0x62
#define ADD_TO_GROUP_3      0x63
#define ADD_TO_GROUP_4      0x64
#define ADD_TO_GROUP_5      0x65
#define ADD_TO_GROUP_6      0x66
#define ADD_TO_GROUP_7      0x67
#define ADD_TO_GROUP_8      0x68
#define ADD_TO_GROUP_9      0x69
#define ADD_TO_GROUP_A      0x6A
#define ADD_TO_GROUP_B      0x6B
#define ADD_TO_GROUP_C      0x6C
#define ADD_TO_GROUP_D      0x6D
#define ADD_TO_GROUP_E      0x6E
#define ADD_TO_GROUP_F      0x6F

#define REMOVE_FROM_GROUP_0   0x70
#define REMOVE_FROM_GROUP_1   0x71
#define REMOVE_FROM_GROUP_2   0x72
#define REMOVE_FROM_GROUP_3   0x73
#define REMOVE_FROM_GROUP_4   0x74
#define REMOVE_FROM_GROUP_5   0x75
#define REMOVE_FROM_GROUP_6   0x76
#define REMOVE_FROM_GROUP_7   0x77
#define REMOVE_FROM_GROUP_8   0x78
#define REMOVE_FROM_GROUP_9   0x79
#define REMOVE_FROM_GROUP_A   0x7A
#define REMOVE_FROM_GROUP_B   0x7B
#define REMOVE_FROM_GROUP_C   0x7C
#define REMOVE_FROM_GROUP_D   0x7D
#define REMOVE_FROM_GROUP_E   0x7E
#define REMOVE_FROM_GROUP_F   0x7F

#define STORE_THE_DTR_AS_SHORT_ADDRESS  0x80
//#define ENABLE_WRITE_MEMORY   0x81

/* Commands 130 through 143 reserved */
/* Queries */
#define QUERY_STATUS    144
#define QUERY_CONTROL_GEAR  145
#define QUERY_LAMP_FAILURE  146
#define QUERY_LAMP_POWER_ON 147
#define QUERY_LIMIT_ERROR 148
#define QUERY_RESET_STATE 149
#define QUERY_MISSING_SHORT_ADDRESS 150
#define QUERY_VERSION_NUMBER  151
#define QUERY_CONTENT_DTR 152
#define QUERY_DEVICE_TYPE 153
#define QUERY_PHYSICAL_MINIMUM  154
#define QUERY_POWER_FAILURE 155
#define QUERY_CONTENT_DTR1  156
#define QUERY_CONTENT_DTR2  157

#define QUERY_ACTUAL_LEVEL  160
#define QUERY_MAX_LEVEL   161
#define QUERY_MIN_LEVEL   162
#define QUERY_POWER_ON_LEVEL  163
#define QUERY_SYSTEM_FAILUE_LEVEL 164 
#define QUERY_FADE_LEVEL  165

/* 166-175 are reserved */

#define QUERY_SCENE_LEVEL_0 176
#define QUERY_SCENE_LEVEL_1 177
#define QUERY_SCENE_LEVEL_2 178
#define QUERY_SCENE_LEVEL_3 179
#define QUERY_SCENE_LEVEL_4 180
#define QUERY_SCENE_LEVEL_5 181
#define QUERY_SCENE_LEVEL_6 182
#define QUERY_SCENE_LEVEL_7 183
#define QUERY_SCENE_LEVEL_8 184
#define QUERY_SCENE_LEVEL_9 185
#define QUERY_SCENE_LEVEL_A 186
#define QUERY_SCENE_LEVEL_B 187
#define QUERY_SCENE_LEVEL_C 188
#define QUERY_SCENE_LEVEL_D 189
#define QUERY_SCENE_LEVEL_E 190
#define QUERY_SCENE_LEVEL_F 191
#define QUERY_GROUPS_0_7  192
#define QUERY_GROUPS_8_F  193
#define QUERY_RANDOM_ADDR_H 194
#define QUERY_RANDOM_ADDR_M 195
#define QUERY_RANDOM_ADDR_L 196
#define READ_MEMORY_LOCATION  197

#define STORE_DTR_AS_FAST_FADE_TIME 228

#define QUERY_GEAR_TYPE       237
#define QUERY_DIMMING_CURVE     238
#define QUERY_POSSIBLE_OPERATING_MODES  239
#define QUERY_FEATURES        240
#define QUERY_FAILURE_STATUS    241

#define QUERY_OPERATING_MODE    252
#define QUERY_FAST_FADE_TIME    253
#define QUERY_MIN_FAST_FADE_TIME  254
#define QUERY_EXTENDED_VERSION_NUMBER 255

#endif /*COMMAND_H_*/
