/**************************************************************************************************
  Filename:       bsp_board.c
  Revised:        $Date: 2009-10-11 16:48:20 -0700 (Sun, 11 Oct 2009) $
  Revision:       $Revision: 20896 $

  Copyright 2008-2009 Texas Instruments Incorporated.  All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights granted under
  the terms of a software license agreement between the user who downloaded the software,
  his/her employer (which must be your employer) and Texas Instruments Incorporated (the
  "License"). You may not use this Software unless you agree to abide by the terms of the
  License. The License limits your use, and you acknowledge, that the Software may not be
  modified, copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio frequency
  transceiver, which is integrated into your product. Other than for the foregoing purpose,
  you may not use, reproduce, copy, prepare derivative works of, modify, distribute,
  perform, display or sell this Software and/or its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS”
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY
  WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
  IN NO EVENT SHALL TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE
  THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY
  INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST
  DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
  THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
**************************************************************************************************/

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 *   BSP (Board Support Package)
 *   Target : Texas Instruments CC430EM
 *   Top-level board code file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

/* ------------------------------------------------------------------------------------------------
 *                                            Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp.h"
#include "bsp_config.h"

/* ------------------------------------------------------------------------------------------------
 *                                            Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static void SetVCore(uint8_t level);
static void SetVCoreUp(uint8_t level);
static void SetVCoreDown(uint8_t level);
static void Bsp_SetVCore(void);
static void Bsp_SetClocks(void);

/* ------------------------------------------------------------------------------------------------
 *                                            Defines
 * ------------------------------------------------------------------------------------------------
 */
#define BSP_TIMER_CLK_MHZ   12       /* 12 MHz MCLKC and SMCLK */
#define BSP_DELAY_MAX_USEC  (0xFFFF/BSP_TIMER_CLK_MHZ)

/**************************************************************************************************
 * @fn          SetVCore
 *
 * @brief       SetVCore level is called from the user API. Change level by one step only.
 *
 * @param       level - VcCore level
 *
 * @return      none
 **************************************************************************************************
 */
static void SetVCore (uint8_t level)
{
  uint8_t actLevel;
  do {
    actLevel = PMMCTL0_L & PMMCOREV_3;
    if (actLevel < level)
      SetVCoreUp(++actLevel);               /* Set VCore (step by step) */
    if (actLevel > level)
      SetVCoreDown(--actLevel);             /* Set VCore (step by step) */
  }while (actLevel != level);
}

/**************************************************************************************************
 * @fn          SetVCoreUp
 *
 * @brief       Set VCore up. Change level by one step only.
 *
 * @param       level - VcCore level
 *
 * @return      none
 *************************************************************************************************
 */
static void SetVCoreUp (uint8_t level)
{
  PMMCTL0_H = 0xA5;                          /* Open PMM module registers for write access */

  SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level; /* Set SVS/M high side to new level */

  SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level; /* Set SVM new Level */
  while ((PMMIFG & SVSMLDLYIFG) == 0);       /* Wait till SVM is settled (Delay) */
  PMMCTL0_L = PMMCOREV0 * level;             /* Set VCore to x */
  PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);         /* Clear already set flags */
  if ((PMMIFG & SVMLIFG))
    while ((PMMIFG & SVMLVLRIFG) == 0);      /* Wait till level is reached */

  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;  /* Set SVS/M Low side to new level */
  PMMCTL0_H = 0x00;                          /* Lock PMM module registers for write access */
}

/**************************************************************************************************
 * @fn          SetVCoreDown
 *
 * @brief       Set VCore down
 *
 * @param       level - VcCore level
 *
 * @return      none
 **************************************************************************************************
 */
static void SetVCoreDown (uint8_t level)
{
  PMMCTL0_H = 0xA5;                         /* Open PMM module registers for write access */
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level; /* Set SVS/M Low side to new level */
  while ((PMMIFG & SVSMLDLYIFG) == 0);      /* Wait till SVM is settled (Delay) */
  PMMCTL0_L = (level * PMMCOREV0);          /* Set VCore to 1.85 V for Max Speed. */
  PMMCTL0_H = 0x00;                         /* Lock PMM module registers for write access */
}

/**************************************************************************************************
 * @fn          Bsp_SetVCore
 *
 * @brief       Setup the core voltage.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Bsp_SetVCore(void)
{
  SetVCore(3);
}

/**************************************************************************************************
 * @fn          Bsp_SetClocks
 *
 * @brief       Set up system clocks.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Bsp_SetClocks(void)
{
  /* Configure CPU clock for 12MHz */

  /* If clock settings are changed, remember to update BSP_TIMER_CLK_MHZ.
   * Otherwise, all timer settings would be incorrect.
   */
  UCSCTL3 |= SELREF_2;                      /* Set DCO FLL reference = REFO */
  UCSCTL4 |= SELA_2;                        /* Set ACLK = REFO */

  __bis_SR_register(SCG0);                  /* Disable the FLL control loop */
  UCSCTL0 = 0x0000;                         /* Set lowest possible DCOx, MODx */
  UCSCTL1 = DCORSEL_5;                      /* Select DCO range 24MHz operation */
  UCSCTL2 = FLLD_1 + 374;                   /* Set DCO Multiplier for 12MHz */
                                            /* (N + 1) * FLLRef = Fdco */
                                            /* (374 + 1) * 32768 = 12MHz */
                                            /* Set FLL Div = fDCOCLK/2 */
  __bic_SR_register(SCG0);                  /* Enable the FLL control loop */

  /* Worst-case settling time for the DCO when the DCO range bits have been
   * changed is n x 32 x 32 x f_MCLK / f_FLL_reference.
   * 32 x 32 x 12 MHz / 32,768 Hz = 375000 = MCLK cycles for DCO to settle
   */
  __delay_cycles(375000);
	
  /* Loop until XT1,XT2 & DCO fault flag is cleared */
  do
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
                                            /* Clear XT2,XT1,DCO fault flags */
    SFRIFG1 &= ~OFIFG;                      /* Clear fault flags */
  }while (SFRIFG1&OFIFG);                   /* Test oscillator fault flag */

  /* Select REFO as ACLK source and DCOCLK as MCLK and SMCLK source */
  UCSCTL4 = SELA__REFOCLK | SELS__DCOCLKDIV | SELM__DCOCLKDIV;
}

/**************************************************************************************************
 * @fn          BSP_EARLY_INIT
 *
 * @brief       This function is called by start-up code before doing the normal initialization
 *              of data segments. If the return value is zero, initialization is not performed.
 *              The global macro label "BSP_EARLY_INIT" gets #defined in the bsp_msp430_defs.h
 *              file, according to the specific compiler environment (CCE or IAR). In the CCE
 *              environment this macro invokes "_system_pre_init()" and in the IAR environment
 *              this macro invokes "__low_level_init()".
 *
 * @param       None
 *
 * @return      0 - don't intialize data segments / 1 - do initialization
 *************************************************************************************************
 */
BSP_EARLY_INIT(void)
{
  /* Disable watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  /* Setup Vcore */
  Bsp_SetVCore();

  /* Configure System clocks */
  Bsp_SetClocks();

  /* Return 1 - run seg_init */
  return (1);
}

/**************************************************************************************************
 * @fn          BSP_InitBoard
 *
 * @brief       Initialize the board.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void BSP_InitBoard(void)
{
  /* Configure TimerA for use by the delay function */

  /* Reset the timer */
  //TA0CTL |= TACLR; /* Set the TACLR  */

  /* Clear all settings */
  //TA0CTL = 0x0;

  /* Select the clk source to be - SMCLK (Sub-Main CLK)*/
  //TA0CTL |= TASSEL_2;
  
  // [BM] We need to use TA1 for delay function, because TA0 is already occupied
  TA1CTL |= TACLR; /* Set the TACLR  */
  TA1CTL = 0x0;
  TA1CTL |= TASSEL_2;
}

/**************************************************************************************************
 * @fn          BSP_Delay
 *
 * @brief       Delay for the requested amount of time.
 *
 * @param       # of microseconds to delay.
 *
 * @return      none
 **************************************************************************************************
 */
void BSP_Delay(uint16_t usec)
{
  BSP_ASSERT(usec < BSP_DELAY_MAX_USEC);

  //TA0R = 0; /* initial count  */
  //TA0CCR0 = BSP_TIMER_CLK_MHZ*usec; /* compare count. (delay in ticks) */

  /* Start the timer in UP mode */
  //TA0CTL |= MC_1;

  /* Loop till compare interrupt flag is set */
  //while(!(TA0CCTL0 & CCIFG));

  /* Stop the timer */
  //TA0CTL &= ~(MC_1);

  /* Clear the interrupt flag */
   //TA0CCTL0 &= ~CCIFG;
   
  // [BM] We need to use TA1 for delay function, because TA0 is already occupied
  TA1R = 0; 
  TA1CCR0 = BSP_TIMER_CLK_MHZ*usec; 
  TA1CTL |= MC_1;
  while(!(TA1CCTL0 & CCIFG));
  TA1CTL &= ~(MC_1);
  TA1CCTL0 &= ~CCIFG;  
}
