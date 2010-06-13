/**************************************************************************************************
  Filename:       nwk_link.h
  Revised:        $Date: 2008-12-10 16:52:14 -0800 (Wed, 10 Dec 2008) $
  Revision:       $Revision: 18596 $
  Author:         $Author: lfriedman $

  Description:    This header file supports the SimpliciTI Join network application.

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

#ifndef NWK_LINK_H
#define NWK_LINK_H

/* Macros needed for protocol backward compatibility */
#define LINK_LEGACY_MSG_LENGTH       8
#define LINK_REPLY_LEGACY_MSG_LENGTH 3


#define LINK_LISTEN_ON   0
#define LINK_LISTEN_OFF  1

/* application payload offsets */
/*    both */
#define LB_REQ_OS         0
#define LB_TID_OS         1

/*    link frame */
#define L_LINK_TOKEN_OS        2
#define L_RMT_PORT_OS          6
#define L_MY_RXTYPE_OS         7
#define L_PROTOCOL_VERSION_OS  8
#define L_CTR_OS               9
/*    link reply frame */
#define LR_RMT_PORT_OS         2
#define LR_MY_RXTYPE_OS        3
#define LR_CTR_OS              4

/*    unlink frame */
#define UL_RMT_PORT_OS        2
/*    unlink reply frame */
#define ULR_RESULT_OS         2

/* change the following as protocol developed */
#ifndef SMPL_SECURE
#define MAX_LINK_APP_FRAME      9
#else
#define MAX_LINK_APP_FRAME      13
#endif

/* frame sizes */
#ifndef SMPL_SECURE
#define LINK_FRAME_SIZE         9
#define LINK_REPLY_FRAME_SIZE   4
#else
#define LINK_FRAME_SIZE         13
#define LINK_REPLY_FRAME_SIZE   8
#endif
#define UNLINK_FRAME_SIZE       3
#define UNLINK_REPLY_FRAME_SIZE 3

/* link requests
 * NOTE: If aditional command codes are required do _not_ use the
 *       value LINK_REPLY_LEGACY_MSG_LENGTH. This numeral is used
 *       to guarantee that legacy Link frames (from before release
 *       1.0.6) work correctly. Don't ask.
 */

#define LINK_REQ_LINK       1
#define LINK_REQ_UNLINK     2

/* prototypes */
fhStatus_t   nwk_processLink(mrfiPacket_t *);
linkID_t     nwk_getLocalLinkID(void);
void         nwk_linkInit(void);
smplStatus_t nwk_link(linkID_t *);
smplStatus_t nwk_unlink(linkID_t);
void         nwk_setLinkToken(uint32_t);
void         nwk_getLinkToken(uint32_t *);
void         nwk_setListenContext(uint8_t);

#endif
