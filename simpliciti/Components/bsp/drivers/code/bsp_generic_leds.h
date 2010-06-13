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
 *   Generic LED macro include file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

#ifndef BSP_GENERIC_LEDS_H
#define BSP_GENERIC_LEDS_H


/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp_macros.h"


/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */

/*
 *  Note : The conditionals in the following macros evaulate compile time constants.
 *         Any compiler worth its salt will optimize these conditionals out for the
 *         smallest possible code.
 */

#define __bsp_LED_TURN_ON__(bit,port,ddr,low)  \
  st( if (low) { port &= ~BV(bit); } else { port |= BV(bit); } )

#define __bsp_LED_TURN_OFF__(bit,port,ddr,low)  \
  st( if (low) { port |= BV(bit); } else { port &= ~BV(bit); } )

#define __bsp_LED_IS_ON__(bit,port,ddr,low)  \
  ( (low) ? (!((port) & BV(bit))) : ((port) & BV(bit)) )

#define __bsp_LED_TOGGLE__(bit,port,ddr,low)     st( port ^= BV(bit); )
#define __bsp_LED_CONFIG__(bit,port,ddr,low)     st( ddr |= BV(bit); )



/* ------------------------- populate LED1 macros ------------------------- */
#define __bsp_NUM_LED1_DEFINES__  ((defined __bsp_LED1_BIT__)  + \
                                   (defined __bsp_LED1_PORT__) + \
                                   (defined __bsp_LED1_DDR__)  + \
                                   (defined __bsp_LED1_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED1_DEFINES__ == 4)
#define __bsp_LED1_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED1_BIT__, __bsp_LED1_PORT__, __bsp_LED1_DDR__, __bsp_LED1_IS_ACTIVE_LOW__ )
#define __bsp_LED1_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED1_BIT__, __bsp_LED1_PORT__, __bsp_LED1_DDR__, __bsp_LED1_IS_ACTIVE_LOW__ )
#define __bsp_LED1_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED1_BIT__, __bsp_LED1_PORT__, __bsp_LED1_DDR__, __bsp_LED1_IS_ACTIVE_LOW__ )
#define __bsp_LED1_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED1_BIT__, __bsp_LED1_PORT__, __bsp_LED1_DDR__, __bsp_LED1_IS_ACTIVE_LOW__ )
#define __bsp_LED1_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED1_BIT__, __bsp_LED1_PORT__, __bsp_LED1_DDR__, __bsp_LED1_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED1_DEFINES__ == 0)
#define __bsp_LED1_TURN_ON__()    /* no LED */
#define __bsp_LED1_TURN_OFF__()   /* no LED */
#define __bsp_LED1_TOGGLE__()     /* no LED */
#define __bsp_LED1_IS_ON__()      /* no LED */  0
#define __bsp_LED1_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED1."
#endif


/* ------------------------- populate LED2 macros ------------------------- */
#define __bsp_NUM_LED2_DEFINES__  ((defined __bsp_LED2_BIT__)  + \
                                   (defined __bsp_LED2_PORT__) + \
                                   (defined __bsp_LED2_DDR__)  + \
                                   (defined __bsp_LED2_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED2_DEFINES__ == 4)
#define __bsp_LED2_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED2_BIT__, __bsp_LED2_PORT__, __bsp_LED2_DDR__, __bsp_LED2_IS_ACTIVE_LOW__ )
#define __bsp_LED2_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED2_BIT__, __bsp_LED2_PORT__, __bsp_LED2_DDR__, __bsp_LED2_IS_ACTIVE_LOW__ )
#define __bsp_LED2_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED2_BIT__, __bsp_LED2_PORT__, __bsp_LED2_DDR__, __bsp_LED2_IS_ACTIVE_LOW__ )
#define __bsp_LED2_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED2_BIT__, __bsp_LED2_PORT__, __bsp_LED2_DDR__, __bsp_LED2_IS_ACTIVE_LOW__ )
#define __bsp_LED2_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED2_BIT__, __bsp_LED2_PORT__, __bsp_LED2_DDR__, __bsp_LED2_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED2_DEFINES__ == 0)
#define __bsp_LED2_TURN_ON__()    /* no LED */
#define __bsp_LED2_TURN_OFF__()   /* no LED */
#define __bsp_LED2_TOGGLE__()     /* no LED */
#define __bsp_LED2_IS_ON__()      /* no LED */  0
#define __bsp_LED2_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED2."
#endif


/* ------------------------- populate LED3 macros ------------------------- */
#define __bsp_NUM_LED3_DEFINES__  ((defined __bsp_LED3_BIT__)  + \
                                   (defined __bsp_LED3_PORT__) + \
                                   (defined __bsp_LED3_DDR__)  + \
                                   (defined __bsp_LED3_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED3_DEFINES__ == 4)
#define __bsp_LED3_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED3_BIT__, __bsp_LED3_PORT__, __bsp_LED3_DDR__, __bsp_LED3_IS_ACTIVE_LOW__ )
#define __bsp_LED3_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED3_BIT__, __bsp_LED3_PORT__, __bsp_LED3_DDR__, __bsp_LED3_IS_ACTIVE_LOW__ )
#define __bsp_LED3_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED3_BIT__, __bsp_LED3_PORT__, __bsp_LED3_DDR__, __bsp_LED3_IS_ACTIVE_LOW__ )
#define __bsp_LED3_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED3_BIT__, __bsp_LED3_PORT__, __bsp_LED3_DDR__, __bsp_LED3_IS_ACTIVE_LOW__ )
#define __bsp_LED3_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED3_BIT__, __bsp_LED3_PORT__, __bsp_LED3_DDR__, __bsp_LED3_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED3_DEFINES__ == 0)
#define __bsp_LED3_TURN_ON__()    /* no LED */
#define __bsp_LED3_TURN_OFF__()   /* no LED */
#define __bsp_LED3_TOGGLE__()     /* no LED */
#define __bsp_LED3_IS_ON__()      /* no LED */  0
#define __bsp_LED3_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED3."
#endif


/* ------------------------- populate LED4 macros ------------------------- */
#define __bsp_NUM_LED4_DEFINES__  ((defined __bsp_LED4_BIT__)  + \
                                   (defined __bsp_LED4_PORT__) + \
                                   (defined __bsp_LED4_DDR__)  + \
                                   (defined __bsp_LED4_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED4_DEFINES__ == 4)
#define __bsp_LED4_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED4_BIT__, __bsp_LED4_PORT__, __bsp_LED4_DDR__, __bsp_LED4_IS_ACTIVE_LOW__ )
#define __bsp_LED4_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED4_BIT__, __bsp_LED4_PORT__, __bsp_LED4_DDR__, __bsp_LED4_IS_ACTIVE_LOW__ )
#define __bsp_LED4_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED4_BIT__, __bsp_LED4_PORT__, __bsp_LED4_DDR__, __bsp_LED4_IS_ACTIVE_LOW__ )
#define __bsp_LED4_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED4_BIT__, __bsp_LED4_PORT__, __bsp_LED4_DDR__, __bsp_LED4_IS_ACTIVE_LOW__ )
#define __bsp_LED4_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED4_BIT__, __bsp_LED4_PORT__, __bsp_LED4_DDR__, __bsp_LED4_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED4_DEFINES__ == 0)
#define __bsp_LED4_TURN_ON__()    /* no LED */
#define __bsp_LED4_TURN_OFF__()   /* no LED */
#define __bsp_LED4_TOGGLE__()     /* no LED */
#define __bsp_LED4_IS_ON__()      /* no LED */  0
#define __bsp_LED4_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED4."
#endif

/* ------------------------- populate LED5 macros ------------------------- */
#define __bsp_NUM_LED5_DEFINES__  ((defined __bsp_LED5_BIT__)  + \
                                   (defined __bsp_LED5_PORT__) + \
                                   (defined __bsp_LED5_DDR__)  + \
                                   (defined __bsp_LED5_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED5_DEFINES__ == 4)
#define __bsp_LED5_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED5_BIT__, __bsp_LED5_PORT__, __bsp_LED5_DDR__, __bsp_LED5_IS_ACTIVE_LOW__ )
#define __bsp_LED5_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED5_BIT__, __bsp_LED5_PORT__, __bsp_LED5_DDR__, __bsp_LED5_IS_ACTIVE_LOW__ )
#define __bsp_LED5_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED5_BIT__, __bsp_LED5_PORT__, __bsp_LED5_DDR__, __bsp_LED5_IS_ACTIVE_LOW__ )
#define __bsp_LED5_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED5_BIT__, __bsp_LED5_PORT__, __bsp_LED5_DDR__, __bsp_LED5_IS_ACTIVE_LOW__ )
#define __bsp_LED5_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED5_BIT__, __bsp_LED5_PORT__, __bsp_LED5_DDR__, __bsp_LED5_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED5_DEFINES__ == 0)
#define __bsp_LED5_TURN_ON__()    /* no LED */
#define __bsp_LED5_TURN_OFF__()   /* no LED */
#define __bsp_LED5_TOGGLE__()     /* no LED */
#define __bsp_LED5_IS_ON__()      /* no LED */  0
#define __bsp_LED5_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED5."
#endif

/* ------------------------- populate LED6 macros ------------------------- */
#define __bsp_NUM_LED6_DEFINES__  ((defined __bsp_LED6_BIT__)  + \
                                   (defined __bsp_LED6_PORT__) + \
                                   (defined __bsp_LED6_DDR__)  + \
                                   (defined __bsp_LED6_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED6_DEFINES__ == 4)
#define __bsp_LED6_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED6_BIT__, __bsp_LED6_PORT__, __bsp_LED6_DDR__, __bsp_LED6_IS_ACTIVE_LOW__ )
#define __bsp_LED6_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED6_BIT__, __bsp_LED6_PORT__, __bsp_LED6_DDR__, __bsp_LED6_IS_ACTIVE_LOW__ )
#define __bsp_LED6_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED6_BIT__, __bsp_LED6_PORT__, __bsp_LED6_DDR__, __bsp_LED6_IS_ACTIVE_LOW__ )
#define __bsp_LED6_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED6_BIT__, __bsp_LED6_PORT__, __bsp_LED6_DDR__, __bsp_LED6_IS_ACTIVE_LOW__ )
#define __bsp_LED6_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED6_BIT__, __bsp_LED6_PORT__, __bsp_LED6_DDR__, __bsp_LED6_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED6_DEFINES__ == 0)
#define __bsp_LED6_TURN_ON__()    /* no LED */
#define __bsp_LED6_TURN_OFF__()   /* no LED */
#define __bsp_LED6_TOGGLE__()     /* no LED */
#define __bsp_LED6_IS_ON__()      /* no LED */  0
#define __bsp_LED6_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED6."
#endif

/* ------------------------- populate LED7 macros ------------------------- */
#define __bsp_NUM_LED7_DEFINES__  ((defined __bsp_LED7_BIT__)  + \
                                   (defined __bsp_LED7_PORT__) + \
                                   (defined __bsp_LED7_DDR__)  + \
                                   (defined __bsp_LED7_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED7_DEFINES__ == 4)
#define __bsp_LED7_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED7_BIT__, __bsp_LED7_PORT__, __bsp_LED7_DDR__, __bsp_LED7_IS_ACTIVE_LOW__ )
#define __bsp_LED7_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED7_BIT__, __bsp_LED7_PORT__, __bsp_LED7_DDR__, __bsp_LED7_IS_ACTIVE_LOW__ )
#define __bsp_LED7_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED7_BIT__, __bsp_LED7_PORT__, __bsp_LED7_DDR__, __bsp_LED7_IS_ACTIVE_LOW__ )
#define __bsp_LED7_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED7_BIT__, __bsp_LED7_PORT__, __bsp_LED7_DDR__, __bsp_LED7_IS_ACTIVE_LOW__ )
#define __bsp_LED7_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED7_BIT__, __bsp_LED7_PORT__, __bsp_LED7_DDR__, __bsp_LED7_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED7_DEFINES__ == 0)
#define __bsp_LED7_TURN_ON__()    /* no LED */
#define __bsp_LED7_TURN_OFF__()   /* no LED */
#define __bsp_LED7_TOGGLE__()     /* no LED */
#define __bsp_LED7_IS_ON__()      /* no LED */  0
#define __bsp_LED7_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED7."
#endif

/* ------------------------- populate LED8 macros ------------------------- */
#define __bsp_NUM_LED8_DEFINES__  ((defined __bsp_LED8_BIT__)  + \
                                   (defined __bsp_LED8_PORT__) + \
                                   (defined __bsp_LED8_DDR__)  + \
                                   (defined __bsp_LED8_IS_ACTIVE_LOW__))
#if (__bsp_NUM_LED8_DEFINES__ == 4)
#define __bsp_LED8_TURN_ON__()    __bsp_LED_TURN_ON__ ( __bsp_LED8_BIT__, __bsp_LED8_PORT__, __bsp_LED8_DDR__, __bsp_LED8_IS_ACTIVE_LOW__ )
#define __bsp_LED8_TURN_OFF__()   __bsp_LED_TURN_OFF__( __bsp_LED8_BIT__, __bsp_LED8_PORT__, __bsp_LED8_DDR__, __bsp_LED8_IS_ACTIVE_LOW__ )
#define __bsp_LED8_TOGGLE__()     __bsp_LED_TOGGLE__  ( __bsp_LED8_BIT__, __bsp_LED8_PORT__, __bsp_LED8_DDR__, __bsp_LED8_IS_ACTIVE_LOW__ )
#define __bsp_LED8_IS_ON__()      __bsp_LED_IS_ON__   ( __bsp_LED8_BIT__, __bsp_LED8_PORT__, __bsp_LED8_DDR__, __bsp_LED8_IS_ACTIVE_LOW__ )
#define __bsp_LED8_CONFIG__()     __bsp_LED_CONFIG__  ( __bsp_LED8_BIT__, __bsp_LED8_PORT__, __bsp_LED8_DDR__, __bsp_LED8_IS_ACTIVE_LOW__ )
#elif (__bsp_NUM_LED8_DEFINES__ == 0)
#define __bsp_LED8_TURN_ON__()    /* no LED */
#define __bsp_LED8_TURN_OFF__()   /* no LED */
#define __bsp_LED8_TOGGLE__()     /* no LED */
#define __bsp_LED8_IS_ON__()      /* no LED */  0
#define __bsp_LED8_CONFIG__()     /* no LED */
#else
#error "ERROR: Incomplete number of macros for LED8."
#endif


/* ************************************************************************************************
 *                                   Compile Time Integrity Checks
 * ************************************************************************************************
 */

/* -------------------- number of LEDs defined -------------------- */
#ifndef __bsp_NUM_LEDS__
#error "ERROR: Number of LEDs is not specified."
#else
#if ((__bsp_NUM_LEDS__ > 8) || (__bsp_NUM_LEDS__ < 0))
#error "ERROR: Unsupported number of LEDs specified.  Maximum is eight."
#endif
#endif

#if (((__bsp_NUM_LED1_DEFINES__ != 0) + \
      (__bsp_NUM_LED2_DEFINES__ != 0) + \
      (__bsp_NUM_LED3_DEFINES__ != 0) + \
      (__bsp_NUM_LED4_DEFINES__ != 0) + \
      (__bsp_NUM_LED5_DEFINES__ != 0) + \
      (__bsp_NUM_LED6_DEFINES__ != 0) + \
      (__bsp_NUM_LED7_DEFINES__ != 0) + \
      (__bsp_NUM_LED8_DEFINES__ != 0)) != __bsp_NUM_LEDS__)
#error "ERROR: Inconsistency between defined macros and specified number of LEDs."
#endif

/* -------------------- blink delay loop count -------------------- */
#ifndef __bsp_LED_BLINK_LOOP_COUNT__
#error "ERROR: Blink delay count is missing."
#endif

/**************************************************************************************************
 */
#endif
