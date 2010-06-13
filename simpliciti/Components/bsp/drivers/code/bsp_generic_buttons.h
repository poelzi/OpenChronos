/**************************************************************************************************
  Revised:        $Date: 2007-07-06 11:19:00 -0700 (Fri, 06 Jul 2007) $
  Revision:       $Revision: 13579 $

  Copyright 2007 Texas Instruments Incorporated.  All rights reserved.

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
 *   Generic button macro include file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

#ifndef BSP_GENERIC_BUTTONS_H
#define BSP_GENERIC_BUTTONS_H


/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp_board_defs.h"
#include "bsp_macros.h"


/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */
#define __bsp_BUTTON__(port,bit,low)    ((low) ? (!((port) & BV(bit))) : ((port) & BV(bit)) )


/* ------------------------- populate BUTTON #1 macros ------------------------- */
#define __bsp_NUM_BUTTON1_DEFINES__  ((defined __bsp_BUTTON1_PORT__)  + \
                                      (defined __bsp_BUTTON1_BIT__)   + \
                                      (defined __bsp_BUTTON1_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON1_DEFINES__ == 3)
#define __bsp_BUTTON1__()           __bsp_BUTTON__( __bsp_BUTTON1_PORT__, __bsp_BUTTON1_BIT__, __bsp_BUTTON1_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON1_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON1_DEFINES__ == 0)
#define __bsp_BUTTON1__()           /* no button */   0
#define __bsp_BUTTON1_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON1."
#endif

/* ------------------------- populate BUTTON #2 macros ------------------------- */
#define __bsp_NUM_BUTTON2_DEFINES__  ((defined __bsp_BUTTON2_PORT__)  + \
                                      (defined __bsp_BUTTON2_BIT__)   + \
                                      (defined __bsp_BUTTON2_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON2_DEFINES__ == 3)
#define __bsp_BUTTON2__()           __bsp_BUTTON__( __bsp_BUTTON2_PORT__, __bsp_BUTTON2_BIT__, __bsp_BUTTON2_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON2_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON2_DEFINES__ == 0)
#define __bsp_BUTTON2__()           /* no button */   0
#define __bsp_BUTTON2_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON2."
#endif

/* ------------------------- populate BUTTON #3 macros ------------------------- */
#define __bsp_NUM_BUTTON3_DEFINES__  ((defined __bsp_BUTTON3_PORT__)  + \
                                      (defined __bsp_BUTTON3_BIT__)   + \
                                      (defined __bsp_BUTTON3_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON3_DEFINES__ == 3)
#define __bsp_BUTTON3__()           __bsp_BUTTON__( __bsp_BUTTON3_PORT__, __bsp_BUTTON3_BIT__, __bsp_BUTTON3_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON3_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON3_DEFINES__ == 0)
#define __bsp_BUTTON3__()           /* no button */   0
#define __bsp_BUTTON3_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON3."
#endif

/* ------------------------- populate BUTTON #4 macros ------------------------- */
#define __bsp_NUM_BUTTON4_DEFINES__  ((defined __bsp_BUTTON4_PORT__)  + \
                                      (defined __bsp_BUTTON4_BIT__)   + \
                                      (defined __bsp_BUTTON4_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON4_DEFINES__ == 3)
#define __bsp_BUTTON4__()           __bsp_BUTTON__( __bsp_BUTTON4_PORT__, __bsp_BUTTON4_BIT__, __bsp_BUTTON4_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON4_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON4_DEFINES__ == 0)
#define __bsp_BUTTON4__()           /* no button */   0
#define __bsp_BUTTON4_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON4."
#endif

/* ------------------------- populate BUTTON #5 macros ------------------------- */
#define __bsp_NUM_BUTTON5_DEFINES__  ((defined __bsp_BUTTON5_PORT__)  + \
                                      (defined __bsp_BUTTON5_BIT__)   + \
                                      (defined __bsp_BUTTON5_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON5_DEFINES__ == 3)
#define __bsp_BUTTON5__()           __bsp_BUTTON__( __bsp_BUTTON5_PORT__, __bsp_BUTTON5_BIT__, __bsp_BUTTON5_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON5_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON5_DEFINES__ == 0)
#define __bsp_BUTTON5__()           /* no button */   0
#define __bsp_BUTTON5_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON5."
#endif

/* ------------------------- populate BUTTON #6 macros ------------------------- */
#define __bsp_NUM_BUTTON6_DEFINES__  ((defined __bsp_BUTTON6_PORT__)  + \
                                      (defined __bsp_BUTTON6_BIT__)   + \
                                      (defined __bsp_BUTTON6_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON6_DEFINES__ == 3)
#define __bsp_BUTTON6__()           __bsp_BUTTON__( __bsp_BUTTON6_PORT__, __bsp_BUTTON6_BIT__, __bsp_BUTTON6_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON6_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON6_DEFINES__ == 0)
#define __bsp_BUTTON6__()           /* no button */   0
#define __bsp_BUTTON6_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON6."
#endif

/* ------------------------- populate BUTTON #7 macros ------------------------- */
#define __bsp_NUM_BUTTON7_DEFINES__  ((defined __bsp_BUTTON7_PORT__)  + \
                                      (defined __bsp_BUTTON7_BIT__)   + \
                                      (defined __bsp_BUTTON7_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON7_DEFINES__ == 3)
#define __bsp_BUTTON7__()           __bsp_BUTTON__( __bsp_BUTTON7_PORT__, __bsp_BUTTON7_BIT__, __bsp_BUTTON7_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON7_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON7_DEFINES__ == 0)
#define __bsp_BUTTON7__()           /* no button */   0
#define __bsp_BUTTON7_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON7."
#endif

/* ------------------------- populate BUTTON #8 macros ------------------------- */
#define __bsp_NUM_BUTTON8_DEFINES__  ((defined __bsp_BUTTON8_PORT__)  + \
                                      (defined __bsp_BUTTON8_BIT__)   + \
                                      (defined __bsp_BUTTON8_IS_ACTIVE_LOW__))
#if (__bsp_NUM_BUTTON8_DEFINES__ == 3)
#define __bsp_BUTTON8__()           __bsp_BUTTON__( __bsp_BUTTON8_PORT__, __bsp_BUTTON8_BIT__, __bsp_BUTTON8_IS_ACTIVE_LOW__ )
#define __bsp_BUTTON8_CONFIG__()    /* nothing required */
#elif (__bsp_NUM_BUTTON8_DEFINES__ == 0)
#define __bsp_BUTTON8__()           /* no button */   0
#define __bsp_BUTTON8_CONFIG__()    /* no button */
#else
#error "ERROR: Incomplete number of macros for BUTTON8."
#endif


/* ************************************************************************************************
 *                                   Compile Time Integrity Checks
 * ************************************************************************************************
 */

/* -------------------- number of buttons defined -------------------- */
#ifndef __bsp_NUM_BUTTONS__
#error "ERROR: Number of buttons is not specified."
#else
#if ((__bsp_NUM_BUTTONS__ > 8) || (__bsp_NUM_BUTTONS__ < 0))
#error "ERROR: Unsupported number of buttons specified.  Maximum is eight."
#endif
#endif

#if (((__bsp_NUM_BUTTON1_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON2_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON3_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON4_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON5_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON6_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON7_DEFINES__ != 0) + \
      (__bsp_NUM_BUTTON8_DEFINES__ != 0)) != __bsp_NUM_BUTTONS__)
#error "ERROR: Inconsistency between defined macros and specified number of buttons."
#endif

/* -------------------- debounce macro -------------------- */
#ifndef __bsp_BUTTON_DEBOUNCE_WAIT__
#error "ERROR: Debounce delay macro is missing."
#endif


/**************************************************************************************************
 */
#endif
