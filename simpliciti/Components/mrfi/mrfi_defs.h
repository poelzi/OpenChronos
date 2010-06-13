/**************************************************************************************************
  Revised:        $Date: 2009-01-13 16:32:00 -0700 (Wed, 13 Jan 2009) $
  Revision:       $Revision: 18768 $

  Copyright 2007-2009 Texas Instruments Incorporated.  All rights reserved.

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

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   MRFI (Minimal RF Interface)
 *   Definition and abstraction for radio targets.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */
#ifndef MRFI_DEFS_H
#define MRFI_DEFS_H


/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "bsp.h"


/* ------------------------------------------------------------------------------------------------
 *                                       Common Defines
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_CCA_RETRIES        4

#define MRFI_ASSERT(x)          BSP_ASSERT(x)
#define MRFI_FORCE_ASSERT()     BSP_FORCE_ASSERT()
#define MRFI_ASSERTS_ARE_ON     BSP_ASSERTS_ARE_ON


/* ------------------------------------------------------------------------------------------------
 *                                    Radio Family Assigment
 * ------------------------------------------------------------------------------------------------
 */

/* ------ Radio Family 1 ------ */
#if (defined MRFI_CC1100) /* Sub 1 GHz RF Transceiver */ || \
    (defined MRFI_CC1101) /* Sub 1 GHz RF Transceiver */ || \
    (defined MRFI_CC1100E_470)  /* Sub 1 GHz RF Transceiver (CC1100E Asia) */ || \
    (defined MRFI_CC1100E_950)  /* Sub 1 GHz RF Transceiver (CC1100E Asia) */ || \
    (defined MRFI_CC2500) /* 2.4 GHz RF Transceiver */
#define MRFI_RADIO_FAMILY1

/* ------ Radio Family 2 ------ */
#elif (defined MRFI_CC1110) /* Sub 1 GHz SoC */                     || \
      (defined MRFI_CC1111) /* Sub 1 GHz SoC with USB controller */ || \
      (defined MRFI_CC2510) /* 2.4 GHz SoC */                       || \
      (defined MRFI_CC2511) /* 2.4 GHz SoC with USB controller */
#define MRFI_RADIO_FAMILY2

/* ------ Radio Family 3 ------ */
#elif (defined MRFI_CC2420) /* 2.4 GHz IEEE 802.15.4 RF Transceiver */ || \
      (defined MRFI_CC2520) /* 2.4 GHz IEEE 802.15.4 RF Transceiver */

#define MRFI_RADIO_FAMILY3

/* ------ Radio Family 4 ------ */
#elif (defined MRFI_CC2430) /* 2.4 GHz IEEE 802.15.4 SoC */ || \
      (defined MRFI_CC2431) /* 2.4 GHz IEEE 802.15.4 SoC */
#define MRFI_RADIO_FAMILY4

/* ------ Radio Family 5 ------ */
#elif (defined MRFI_CC430)  /* Sub 1 GHz MSP SoC */
#define MRFI_RADIO_FAMILY5

/* ------ Radio Family 6 ------ */
#elif (defined MRFI_CC2530) /* 2.4 GHz IEEE 802.15.4 SoC */

#define MRFI_RADIO_FAMILY6

#else
#error "ERROR: Unknown or missing radio selection."
#endif


/* ------------------------------------------------------------------------------------------------
 *                                Radio Family 1 / Radio Family 2 / Radio Family 5
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_RADIO_FAMILY1) || (defined MRFI_RADIO_FAMILY2) || (defined MRFI_RADIO_FAMILY5)

#define __mrfi_LENGTH_FIELD_SIZE__      1
#define __mrfi_ADDR_SIZE__              4
#define __mrfi_MAX_PAYLOAD_SIZE__       20

#define __mrfi_RX_METRICS_SIZE__        2
#define __mrfi_RX_METRICS_RSSI_OFS__    0
#define __mrfi_RX_METRICS_CRC_LQI_OFS__ 1
#define __mrfi_RX_METRICS_CRC_OK_MASK__ 0x80
#define __mrfi_RX_METRICS_LQI_MASK__    0x7F

#define __mrfi_NUM_LOGICAL_CHANS__      4
#define __mrfi_NUM_POWER_SETTINGS__     3

#define __mrfi_BACKOFF_PERIOD_USECS__   250

#define __mrfi_LENGTH_FIELD_OFS__       0
#define __mrfi_DST_ADDR_OFS__           (__mrfi_LENGTH_FIELD_OFS__ + __mrfi_LENGTH_FIELD_SIZE__)
#define __mrfi_SRC_ADDR_OFS__           (__mrfi_DST_ADDR_OFS__ + __mrfi_ADDR_SIZE__)
#define __mrfi_PAYLOAD_OFS__            (__mrfi_SRC_ADDR_OFS__ + __mrfi_ADDR_SIZE__)

#define __mrfi_HEADER_SIZE__            (2 * __mrfi_ADDR_SIZE__)
#define __mrfi_FRAME_OVERHEAD_SIZE__    (__mrfi_LENGTH_FIELD_SIZE__ + __mrfi_HEADER_SIZE__)

#define __mrfi_GET_PAYLOAD_LEN__(p)     ((p)->frame[__mrfi_LENGTH_FIELD_OFS__] - __mrfi_HEADER_SIZE__)
#define __mrfi_SET_PAYLOAD_LEN__(p,x)   st( (p)->frame[__mrfi_LENGTH_FIELD_OFS__] = x + __mrfi_HEADER_SIZE__; )

#endif


/* ------------------------------------------------------------------------------------------------
 *                                Radio Family 3 / Radio Family 4 / Radio Family 6
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_RADIO_FAMILY3) || (defined MRFI_RADIO_FAMILY4) || (defined MRFI_RADIO_FAMILY6)

#define __mrfi_LENGTH_FIELD_SIZE__      1
#define __mrfi_FCF_SIZE__               2
#define __mrfi_DSN_SIZE__               1
#define __mrfi_ADDR_SIZE__              4
#define __mrfi_MAX_PAYLOAD_SIZE__       20

#define __mrfi_RX_METRICS_SIZE__        2
#define __mrfi_RX_METRICS_RSSI_OFS__    0
#define __mrfi_RX_METRICS_CRC_LQI_OFS__ 1
#define __mrfi_RX_METRICS_CRC_OK_MASK__ 0x80
#define __mrfi_RX_METRICS_LQI_MASK__    0x7F

#define __mrfi_NUM_LOGICAL_CHANS__      4
#define __mrfi_NUM_POWER_SETTINGS__     3

#define __mrfi_BACKOFF_PERIOD_USECS__   250

#define __mrfi_LENGTH_FIELD_OFS__       0
#define __mrfi_FCF_OFS__                (__mrfi_LENGTH_FIELD_OFS__ +  __mrfi_LENGTH_FIELD_SIZE__)
#define __mrfi_DSN_OFS__                (__mrfi_FCF_OFS__          +  __mrfi_FCF_SIZE__)
#define __mrfi_DST_ADDR_OFS__           (__mrfi_DSN_OFS__          +  __mrfi_DSN_SIZE__)
#define __mrfi_SRC_ADDR_OFS__           (__mrfi_DST_ADDR_OFS__     +  __mrfi_ADDR_SIZE__)
#define __mrfi_PAYLOAD_OFS__            (__mrfi_SRC_ADDR_OFS__     +  __mrfi_ADDR_SIZE__)

#define __mrfi_HEADER_SIZE__            ((2 * __mrfi_ADDR_SIZE__) + __mrfi_FCF_SIZE__ + __mrfi_DSN_SIZE__)
#define __mrfi_FRAME_OVERHEAD_SIZE__    (__mrfi_LENGTH_FIELD_SIZE__ + __mrfi_HEADER_SIZE__)

#define __mrfi_GET_PAYLOAD_LEN__(p)     ((p)->frame[__mrfi_LENGTH_FIELD_OFS__] - __mrfi_HEADER_SIZE__)
#define __mrfi_SET_PAYLOAD_LEN__(p,x)   st( (p)->frame[__mrfi_LENGTH_FIELD_OFS__] = x + __mrfi_HEADER_SIZE__; )

#endif


/* ------------------------------------------------------------------------------------------------
 *                                   Radio Family Commonality
 * ------------------------------------------------------------------------------------------------
 */
#define __mrfi_P_DST_ADDR__(p)          (&((p)->frame[__mrfi_DST_ADDR_OFS__]))
#define __mrfi_P_SRC_ADDR__(p)          (&((p)->frame[__mrfi_SRC_ADDR_OFS__]))
#define __mrfi_P_PAYLOAD__(p)           (&((p)->frame[__mrfi_PAYLOAD_OFS__]))


/* ************************************************************************************************
 *                                   Compile Time Integrity Checks
 * ************************************************************************************************
 */

/* verify that only one supported radio is selected */
#define MRFI_NUM_SUPPORTED_RADIOS_SELECTED   ((defined MRFI_CC1100) + \
                                              (defined MRFI_CC1101) + \
                                              (defined MRFI_CC1110) + \
                                              (defined MRFI_CC1111) + \
                                              (defined MRFI_CC1100E_470) + \
                                              (defined MRFI_CC1100E_950) + \
                                              (defined MRFI_CC2500) + \
                                              (defined MRFI_CC2510) + \
                                              (defined MRFI_CC2511) + \
                                              (defined MRFI_CC2430) + \
                                              (defined MRFI_CC2431) + \
                                              (defined MRFI_CC2520) + \
                                              (defined MRFI_CC430)  + \
                                              (defined MRFI_CC2530))
#if (MRFI_NUM_SUPPORTED_RADIOS_SELECTED == 0)
#error "ERROR: A valid radio is not selected."
#elif (MRFI_NUM_SUPPORTED_RADIOS_SELECTED > 1)
#error "ERROR: More than one radio is selected."
#endif

/* verify that a radio family is selected */
#if (!defined MRFI_RADIO_FAMILY1) && \
    (!defined MRFI_RADIO_FAMILY2) && \
    (!defined MRFI_RADIO_FAMILY3) && \
    (!defined MRFI_RADIO_FAMILY4) && \
    (!defined MRFI_RADIO_FAMILY5) && \
    (!defined MRFI_RADIO_FAMILY6)
#error "ERROR: A radio family has not been assigned."
#endif


/**************************************************************************************************
 */
#endif
