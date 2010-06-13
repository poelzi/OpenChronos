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
 *   LED driver include file.
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 */

#ifndef BSP_LEDS_H
#define BSP_LEDS_H


/* ------------------------------------------------------------------------------------------------
 *                                           Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp_led_defs.h"
#include "bsp_macros.h"


/* ------------------------------------------------------------------------------------------------
 *                                             Defines
 * ------------------------------------------------------------------------------------------------
 */
#define BSP_NUM_LEDS            __bsp_NUM_LEDS__


/* ------------------------------------------------------------------------------------------------
 *                                              Macros
 * ------------------------------------------------------------------------------------------------
 */

/* blink delay loop */
#define BSP_LED_BLINK_DELAY()   st( { volatile uint32_t i; \
                                      for (i=0; i<__bsp_LED_BLINK_LOOP_COUNT__; i++) { }; } )

/* LED1 */
#define BSP_TURN_ON_LED1()      __bsp_LED1_TURN_ON__()
#define BSP_TURN_OFF_LED1()     __bsp_LED1_TURN_OFF__()
#define BSP_TOGGLE_LED1()       __bsp_LED1_TOGGLE__()
#define BSP_LED1_IS_ON()        __bsp_LED1_IS_ON__()

/* LED2 */
#define BSP_TURN_ON_LED2()      __bsp_LED2_TURN_ON__()
#define BSP_TURN_OFF_LED2()     __bsp_LED2_TURN_OFF__()
#define BSP_TOGGLE_LED2()       __bsp_LED2_TOGGLE__()
#define BSP_LED2_IS_ON()        __bsp_LED2_IS_ON__()

/* LED3 */
#define BSP_TURN_ON_LED3()      __bsp_LED3_TURN_ON__()
#define BSP_TURN_OFF_LED3()     __bsp_LED3_TURN_OFF__()
#define BSP_TOGGLE_LED3()       __bsp_LED3_TOGGLE__()
#define BSP_LED3_IS_ON()        __bsp_LED3_IS_ON__()

/* LED4 */
#define BSP_TURN_ON_LED4()      __bsp_LED4_TURN_ON__()
#define BSP_TURN_OFF_LED4()     __bsp_LED4_TURN_OFF__()
#define BSP_TOGGLE_LED4()       __bsp_LED4_TOGGLE__()
#define BSP_LED4_IS_ON()        __bsp_LED4_IS_ON__()

/* LED5 */
#define BSP_TURN_ON_LED5()      __bsp_LED5_TURN_ON__()
#define BSP_TURN_OFF_LED5()     __bsp_LED5_TURN_OFF__()
#define BSP_TOGGLE_LED5()       __bsp_LED5_TOGGLE__()
#define BSP_LED5_IS_ON()        __bsp_LED5_IS_ON__()

/* LED6 */
#define BSP_TURN_ON_LED6()      __bsp_LED6_TURN_ON__()
#define BSP_TURN_OFF_LED6()     __bsp_LED6_TURN_OFF__()
#define BSP_TOGGLE_LED6()       __bsp_LED6_TOGGLE__()
#define BSP_LED6_IS_ON()        __bsp_LED6_IS_ON__()

/* LED7 */
#define BSP_TURN_ON_LED7()      __bsp_LED7_TURN_ON__()
#define BSP_TURN_OFF_LED7()     __bsp_LED7_TURN_OFF__()
#define BSP_TOGGLE_LED7()       __bsp_LED7_TOGGLE__()
#define BSP_LED7_IS_ON()        __bsp_LED7_IS_ON__()

/* LED8 */
#define BSP_TURN_ON_LED8()      __bsp_LED8_TURN_ON__()
#define BSP_TURN_OFF_LED8()     __bsp_LED8_TURN_OFF__()
#define BSP_TOGGLE_LED8()       __bsp_LED8_TOGGLE__()
#define BSP_LED8_IS_ON()        __bsp_LED8_IS_ON__()


/* ------------------------------------------------------------------------------------------------
 *                                        Prototypes
 * ------------------------------------------------------------------------------------------------
 */
void BSP_InitLeds(void);



/* ************************************************************************************************
 *                                   Compile Time Integrity Checks
 * ************************************************************************************************
 */
#ifdef BSP_NO_LEDS
#error "ERROR: The LED driver is disabled.  This file should not be included."
#endif

/**************************************************************************************************
 */
#endif
