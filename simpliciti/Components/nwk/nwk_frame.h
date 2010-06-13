/**************************************************************************************************
  Filename:       nwk_frame.h
  Revised:        $Date: 2008-12-23 13:49:41 -0800 (Tue, 23 Dec 2008) $
  Revision:       $Revision: 18651 $
  Author:         $Author: lfriedman $

  Description:    This header file supports the SimpliciTI frame handling functions.

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


#ifndef NWK_FRAME_H
#define NWK_FRAME_H

/* Frame field defines and masks. Mask name must be field name with '_MSK' appended
 * so the GET and PUT macros work correctly -- they use token pasting. Offset values
 * are with respect to the MRFI payload and not the entire frame.
 */
#define F_PORT_OS         0
#define F_PORT_OS_MSK     (0x3F)
#define F_ENCRYPT_OS      0
#define F_ENCRYPT_OS_MSK  (0x40)
#define F_FWD_FRAME       0
#define F_FWD_FRAME_MSK   (0x80)
#define F_RX_TYPE         1
#define F_RX_TYPE_MSK     (0x40)
#define F_ACK_REQ         1
#define F_ACK_REQ_MSK     (0x80)
#define F_ACK_RPLY        1
#define F_ACK_RPLY_MSK    (0x08)
#define F_TX_DEVICE       1
#define F_TX_DEVICE_MSK   (0x30)
#define F_HOP_COUNT       1
#define F_HOP_COUNT_MSK   (0x07)
#define F_TRACTID_OS      2
#define F_TRACTID_OS_MSK  (0xFF)
#define SMPL_NWK_HDR_SIZE 3

#ifdef SMPL_SECURE

#define F_SECURE_OS       3

#define F_SEC_CTR_OS      3       /* counter hint */
#define F_SEC_CTR_OS_MSK  (0xFF)
#define F_SEC_ICHK_OS     4       /* Message integrity check */
#define F_SEC_ICHK_OS_MSK (0xFF)
#define F_SEC_MAC_OS      5       /* Message authentication code */
#define F_SEC_MAC_OS_MSK  (0xFF)

#else

#define F_SECURE_OS       0

#endif  /* SMPL_SECURE */

#define F_APP_PAYLOAD_OS  (SMPL_NWK_HDR_SIZE+F_SECURE_OS)

/* sub field details. they are in the correct bit locations (already shifted) */
#define F_RX_TYPE_USER_CTL       0x00    /* does not poll... */
#define F_RX_TYPE_POLLS          0x40    /* polls for held messages */

#define F_ACK_REQ_TYPE           0x80
#define F_ACK_RPLY_TYPE          0x08
#define F_FRAME_FWD_TYPE         0x80
#define F_FRAME_ENCRYPT_TYPE     0x40

/* device type fields */
#define F_TX_DEVICE_ED           0x00    /* End Device */
#define F_TX_DEVICE_RE           0x10    /* Range Extender */
#define F_TX_DEVICE_AP           0x20    /* Access Point */

/* macro to get a field from a frame buffer */
#define GET_FROM_FRAME(b,f)  ((b)[f] & (f##_MSK))

/* Macro to put a value 'v' into a frame buffer 'b'. 'v' value must already be shifted
 * if necessary. 'b' is a byte array
 */
#define PUT_INTO_FRAME(b,f,v)  do {(b)[f] = ((b)[f] & ~(f##_MSK)) | (v); } while(0)


/*       ****   frame information objects
 * info kept on each frame object
 */
#define   FI_AVAILABLE         0   /* entry available for use */
#define   FI_INUSE_UNTIL_DEL   1   /* in use. will be explicitly reclaimed */
#define   FI_INUSE_UNTIL_TX    2   /* in use. will be reclaimed after Tx */
#define   FI_INUSE_UNTIL_FWD   3   /* in use until forwarded by AP */
#define   FI_INUSE_TRANSITION  4   /* being retrieved. do not delete in Rx ISR thread. */

typedef struct
{
  uint8_t   rssi;
  uint8_t   lqi;
} sigInfo_t;

typedef struct
{
  volatile uint8_t      fi_usage;
           uint8_t      orderStamp;
           mrfiPacket_t mrfiPkt;
} frameInfo_t;


/* prototypes */
frameInfo_t  *nwk_buildFrame(uint8_t, uint8_t *msg, uint8_t len, uint8_t hops);
#ifdef APP_AUTO_ACK
frameInfo_t  *nwk_buildAckReqFrame(uint8_t, uint8_t *, uint8_t, uint8_t, volatile uint8_t *);
#endif
void          nwk_receiveFrame(void);
void          nwk_frameInit(uint8_t (*)(linkID_t));
smplStatus_t  nwk_retrieveFrame(rcvContext_t *, uint8_t *, uint8_t *, addr_t *, uint8_t *);
smplStatus_t  nwk_sendFrame(frameInfo_t *, uint8_t txOption);
frameInfo_t  *nwk_getSandFFrame(mrfiPacket_t *, uint8_t);
uint8_t       nwk_getMyRxType(void);
void          nwk_SendEmptyPollRspFrame(mrfiPacket_t *);
#ifdef APP_AUTO_ACK
void          nwk_sendAckReply(mrfiPacket_t *, uint8_t);
#endif

#ifndef END_DEVICE
/* only APs and REs repeat frames */
void  nwk_replayFrame(frameInfo_t *);
#endif


#endif
