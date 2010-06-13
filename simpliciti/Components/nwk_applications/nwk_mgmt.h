/**************************************************************************************************
  Filename:       nwk_mgmt.h
  Revised:        $Date: 2009-01-06 15:45:54 -0800 (Tue, 06 Jan 2009) $
  Revision:       $Revision: 18697 $
  Author:         $Author: lfriedman $

  Description:    This header file supports the SimpliciTI Mgmt network application.

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


#ifndef NWK_MGMT_H
#define NWK_MGMT_H

/* MGMT frame application requests */
#define  MGMT_REQ_POLL        0x01

/* change the following as protocol developed */
#define MAX_MGMT_APP_FRAME    7

/* application payload offsets */
/*    both */
#define MB_APP_INFO_OS           0
#define MB_TID_OS                1

/*    Poll frame */
#define M_POLL_PORT_OS          2
#define M_POLL_ADDR_OS          3

/* change the following as protocol developed */
#define MAX_MGMT_APP_FRAME    7

/* frame sizes */
#define MGMT_POLL_FRAME_SIZE  7

/* prototypes */
void         nwk_mgmtInit(void);
fhStatus_t   nwk_processMgmt(mrfiPacket_t *);
smplStatus_t nwk_poll(uint8_t, uint8_t *);
void         nwk_resetSFMarker(uint8_t);

#endif
