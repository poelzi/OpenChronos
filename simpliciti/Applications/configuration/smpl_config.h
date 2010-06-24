/**************************************************************************************************
  Filename:       smpl_config.dat
  Revised:        $Date: 2008-10-02 15:57:27 -0700 (Thu, 02 Oct 2008) $
  Revision:       $Revision: 18187 $
  Author:         $Author: lfriedman $

  Description:    This file supports the SimpliciTI Customer Configuration for End Devices.

  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

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

#include "config.h"

#ifndef SMPL_CONFIG
#define SMPL_CONFIG

/* Number of connections supported. each connection supports bi#define irectional
 * communication.  Access Points and Range Extenders can set this to 0 if they
 * do not host End Device objects
 */
/*#define NUM_CONNECTIONS  2*/
/* [BM] Only allow 1 connection to RF access point */
#define NUM_CONNECTIONS  1

/*  ***  Size of low level queues for sent and received frames. Affects RAM usage  ***  */

/* AP needs larger input frame queue if it is supporting store-and-forward
 * clients because the forwarded messages are held here. Two is probably enough
 * for an End Device
 */
#define SIZE_INFRAME_Q  2

/* The output frame queue can be small since Tx is done synchronously. Actually
 * 1 is probably enough. If an Access Point device is also hosting an End Device 
 * that sends to a sleeping peer the output queue should be larger -- the waiting 
 * frames in this case are held here. In that case the output frame queue should 
 * be bigger. 
 */
#define SIZE_OUTFRAME_Q  2

/* This device's address. The first byte is used as a filter on the CC1100/CC2500
 * radios so THE FIRST BYTE MUST NOT BE either 0x00 or 0xFF. Also, for these radios
 * on End Devices the first byte should be the least significant byte so the filtering
 * is maximally effective. Otherwise the frame has to be processed by the MCU before it
 * is recognized as not intended for the device. APs and REs run in promiscuous mode so
 * the filtering is not done. This macro intializes a static const array of unsigned
 * characters of length NET_ADDR_SIZE (found in nwk_types.h). the quotes (") are
 * necessary below unless the spaces are removed.
 */
// pfs removed double quotes around {0x79, 0x56, 0x34, 0x12}
#ifndef THIS_DEVICE_ADDRESS
#define THIS_DEVICE_ADDRESS  {0x79, 0x56, 0x34, 0x12}
#warning no hardware adress set
#endif

/* device type */
#define END_DEVICE

/* For polling End Devices we need to specify that they do so. Uncomment the 
 * macro definition below if this is a polling device. This field is used 
 * by the Access Point to know whether to reserve store-and-forward support 
 * for the polling End Device during the Join exchange.
 */
/* #define RX_POLLS */

#endif