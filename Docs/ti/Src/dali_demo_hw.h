/******************************************************************************/
// dali_demo_hw.h
// definitions for DALI demo Hardware
//
// Luis R
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
#ifndef DALI_DEMO_HW_H_
#define DALI_DEMO_HW_H_

#include "msp430.h"

//
// The project configuration should define the platform
// Valid configurations:
//  G2xx2_LAUNCHPAD: Use MSP430G2452 with launchpad (MSP-EXP430G2)
//  F2131_TPS62260LED_338EVM: Use MSP430F2131 with TPS62260LED-338 EVM
//

#ifdef G2xx2_LAUNCHPAD
/******************************************************************************/
/********************DEFINITION FOR MSP430G2xx2 USING LAUNCHPAD ***************/
    /**********GPIO Definitions ***************/
    /*! P1.1 = TPS62260 Enable */
    #define TPS62260_ENABLE_PxOUT   P1OUT
    #define TPS62260_ENABLE_BIT     BIT1
    /*! P1.6 = PWM1 using TA0.1 */
    #define PWM1_BIT                BIT6
    /*! P1.4 = PWM2 using TA0.2 */
    #define PWM2_BIT                BIT4
    /*! DALI RX = P2.0*/
    #define DALI_RX_PxIN            P2IN
    #define DALI_RX_PxIES           P2IES
    #define DALI_RX_PxIFG           P2IFG
    #define DALI_RX_BIT             BIT0
    /*! DALI TX = P2.1*/
    #define DALI_TX_PxOUT           P2OUT
    #define DALI_TX_BIT             BIT1
    #define GPIO_INIT() {  P1OUT = 0x00; P1DIR = 0xFF; P1SEL = (PWM1_BIT|PWM2_BIT);  \
        P1SEL2 = PWM2_BIT; P2OUT = DALI_TX_BIT; P2DIR = (0xFF-DALI_RX_BIT);}

    /*********** TimerA Definitions **********/
    #define TIMER_VECTOR    TIMER0_A1_VECTOR
    #define TAxCCR0         TA0CCR0
    #define TAxCCR1         TA0CCR1
    #define TAxCCR2         TA0CCR2
    #define TAxCCTL1        TA0CCTL1
    #define TAxCCTL2        TA0CCTL2
    #define TAxCTL          TA0CTL

    /*********** Define unassigned vectors which will call a dummy ISR *******/
    #define DUMMY_VECTORS  ADC10_VECTOR,COMPARATORA_VECTOR,NMI_VECTOR,\
                            PORT1_VECTOR,PORT2_VECTOR,TIMER0_A0_VECTOR,USI_VECTOR

#elif defined (F2131_TPS62260LED_338EVM)
/******************************************************************************/
/************DEFINITION FOR MSP430F2131 U TPS62260LED_338 EVM *************/
    /**********GPIO Definitions ***************/
    /*! P1.0 = TPS62260 Enable */
    #define TPS62260_ENABLE_PxOUT   P1OUT
    #define TPS62260_ENABLE_BIT     BIT0
    /*! P1.2 = PWM1 using TA0.1 */
    #define PWM1_BIT                BIT2
    /*! P1.3 = PWM2 using TA0.2 */
    #define PWM2_BIT                BIT3
    /*! DALI RX = P2.0*/
    #define DALI_RX_PxIN            P2IN
    #define DALI_RX_PxIES           P2IES
    #define DALI_RX_PxIFG           P2IFG
    #define DALI_RX_BIT             BIT0
    /*! DALI TX = P2.1*/
    #define DALI_TX_PxOUT           P2OUT
    #define DALI_TX_BIT             BIT1
    #define GPIO_INIT() {  P1OUT = BIT1; P1DIR = 0xFF; P1SEL = (PWM1_BIT|PWM2_BIT);  \
       P2OUT = DALI_TX_BIT; P2DIR = (0xFF-DALI_RX_BIT);}

    /*********** TimerA Definitions **********/
    #define TIMER_VECTOR    TIMERA1_VECTOR
    #define TAxCCR0         TACCR0
    #define TAxCCR1         TACCR1
    #define TAxCCR2         TACCR2
    #define TAxCCTL1        TACCTL1
    #define TAxCCTL2        TACCTL2
    #define TAxCTL          TACTL


    /*********** Define unassigned vectors which will call a dummy ISR *******/
    #define DUMMY_VECTORS  COMPARATORA_VECTOR,NMI_VECTOR,\
                            PORT1_VECTOR,PORT2_VECTOR,TIMER0_A0_VECTOR

#else
#error "Define a valid platform in project settings (Pre-defined symbols) or dali_demo_hw.h"
#endif

#endif /*DALI_H_*/
