/**************************************************************************************************
  Filename:       nwk_api.h
  Revised:        $Date: 2008-11-24 12:09:31 -0800 (Mon, 24 Nov 2008) $
  Revision:       $Revision: 18508 $
  Author:         $Author: lfriedman $

  Description:    This header file supports the SimpliciTI appliction layer API.

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

#ifndef NWK_API_H
#define NWK_API_H

/* Tx options (bit map) */
#define  SMPL_TXOPTION_NONE       ((txOpt_t)0x00)
#define  SMPL_TXOPTION_ACKREQ     ((txOpt_t)0x01)

smplStatus_t SMPL_Init(uint8_t (*)(linkID_t));
smplStatus_t SMPL_Link(linkID_t *);
smplStatus_t SMPL_LinkListen(linkID_t *);
smplStatus_t SMPL_Send(linkID_t lid, uint8_t *msg, uint8_t len);
smplStatus_t SMPL_SendOpt(linkID_t lid, uint8_t *msg, uint8_t len, txOpt_t);
smplStatus_t SMPL_Receive(linkID_t lid, uint8_t *msg, uint8_t *len);
smplStatus_t SMPL_Ioctl(ioctlObject_t, ioctlAction_t, void *);
#ifdef EXTENDED_API
smplStatus_t SMPL_Ping(linkID_t);
smplStatus_t SMPL_Unlink(linkID_t);
smplStatus_t SMPL_Commission(addr_t *, uint8_t, uint8_t, linkID_t *);
#endif  /* EXTENDED_API */

#endif
