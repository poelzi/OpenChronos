/**************************************************************************************************
  Filename:       nwk_mgmt.c
  Revised:        $Date: 2009-01-06 15:45:54 -0800 (Tue, 06 Jan 2009) $
  Revision:       $Revision: 18697 $
  Author:         $Author: lfriedman $

  Description:    This file supports the SimpliciTI Mgmt network application.

  Copyright 2007-2009 Texas Instruments Incorporated. All rights reserved.

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


/******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "nwk_frame.h"
#include "nwk.h"
#include "nwk_mgmt.h"
#include "nwk_join.h"
#include "nwk_globals.h"
#include "nwk_QMgmt.h"

/******************************************************************************
 * MACROS
 */

/******************************************************************************
 * CONSTANTS AND DEFINES
 */

/******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * LOCAL VARIABLES
 */
#ifndef ACCESS_POINT
static addr_t const *sAPAddr = NULL;
#else
static uint8_t sSFMarker[NUM_STORE_AND_FWD_CLIENTS] = {0};
#endif

static volatile uint8_t sTid = 0;

/******************************************************************************
 * LOCAL FUNCTIONS
 */
static void  smpl_send_mgmt_reply(mrfiPacket_t *);
#ifdef ACCESS_POINT
static void  send_poll_reply(mrfiPacket_t *);
#endif

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * GLOBAL FUNCTIONS
 */

/******************************************************************************
 * @fn          nwk_mgmtInit
 *
 * @brief       Initialize Management functions.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   void
 */

void nwk_mgmtInit(void)
{
  sTid = MRFI_RandomByte();

#ifdef ACCESS_POINT
  memset(&sSFMarker, 0x0, sizeof(sSFMarker));
#endif

  return;
}

/******************************************************************************
 * @fn          nwk_processMgmt
 *
 * @brief       Process Management frame. Just save the frame for the Management
 *              app it it is a reply. If it isn't a reply, send the reply in this
 *              thread.
 *
 * input parameters
 * @param   frame   - pointer to frame to be processed
 *
 * output parameters
 *
 * @return   Keep frame for application, release frame, or replay frame.
 */
fhStatus_t nwk_processMgmt(mrfiPacket_t *frame)
{
  fhStatus_t   rc;
  uint8_t      replyType;

  /* If we sent this then this is the reply. Validate the
   * packet for reception by client app. If we didn't send
   * it then we are the target. send the reply.
   */
  if (SMPL_MY_REPLY == (replyType=nwk_isValidReply(frame, sTid, MB_APP_INFO_OS, MB_TID_OS)))
  {
    /* It's a match and it's a reply. Validate the received packet by
     * returning a 1 so it can be received by the client app.
     */
    MRFI_PostKillSem();
    rc = FHS_KEEP;
  }
#if !defined( END_DEVICE )
  else if (SMPL_A_REPLY == replyType)
  {
    /* no match. if i'm not an ED this is a reply that should be passed on. */
    rc = FHS_REPLAY;
  }
#endif  /* !END_DEVICE */
  else
  {
    /* no, we didn't send it. send reply if it's intended for us */
    if (!memcmp(MRFI_P_DST_ADDR(frame), nwk_getMyAddress(), NET_ADDR_SIZE))
    {
      smpl_send_mgmt_reply(frame);

      /* we're done with the frame. */
      rc = FHS_RELEASE;
    }
    else
    {
      rc = FHS_REPLAY;
    }
  }

  (void) replyType;  /* keep compiler happy */

  return rc;
}

/******************************************************************************
 * @fn          smpl_send_mgmt_reply
 *
 * @brief       Send appropriate reply to Management frame.
 *
 * input parameters
 * @param  frame  - Pointer to frame for which reply required.
 *
 * output parameters
 *
 * @return   void
 */
static void smpl_send_mgmt_reply(mrfiPacket_t *frame)
{
#ifdef ACCESS_POINT
  /* what kind of management frame is this? */
  switch (*(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+MB_APP_INFO_OS))
  {
    case MGMT_REQ_POLL:
      send_poll_reply(frame);
      break;

    default:
      break;
  }
#endif  /* ACCESS_POINT */
  return;
}


#ifdef ACCESS_POINT
/******************************************************************************
 * @fn          send_poll_reply
 *
 * @brief       Send reply to polling frame.
 *
 * input parameters
 * @param  frame  - Pointer to frame for which reply required.
 *
 * output parameters
 *
 * @return   void
 */
static void send_poll_reply(mrfiPacket_t *frame)
{
  uint8_t         msgtid = *(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+MB_TID_OS);
  frameInfo_t    *pOutFrame;
  sfClientInfo_t *pClientInfo;
  uint8_t         loc;

  /* Make sure this guy is really a client. We can tell from the source address. */
  if (!(pClientInfo=nwk_isSandFClient(MRFI_P_SRC_ADDR(frame), &loc)))
  {
    /* TODO: maybe send an error frame? */
    return;
  }

  /* If we have to resync the TID then do it based on the first
   * poll frame we see
  */
  if (!sSFMarker[loc])
  {
    /* If the marker flag is null then it has been initialized, i.e.,
     * there has been a reset. In this case infer that we need to update
     * a (probably) stale last TID. The test will always be true the first
     * time through after a client is established even when an NV restore
     * did not take place but this does no harm.
     */
    pClientInfo->lastTID = msgtid;
    sSFMarker[loc]       = 1;
  }
  /* If we've seen this poll frame before ignore it. Otherwise we
   * may send a stored frame when we shouldn't.
   */
  else if (nwk_checkAppMsgTID(pClientInfo->lastTID, msgtid))
  {
    pClientInfo->lastTID = msgtid;
  }
  else
  {
    return;
  }

  if (pOutFrame = nwk_getSandFFrame(frame, M_POLL_PORT_OS))
  {
    /* We need to adjust the order in the queue in this case. Currently
     * we know it is in the input queue and that this adjustment is safe
     * because we're in an ISR thread. This is a fragile fix, though, and
     * should be revisited when time permits.
     */
    nwk_QadjustOrder(INQ, pOutFrame->orderStamp);

    /* reset hop count... */
    PUT_INTO_FRAME(MRFI_P_PAYLOAD(&pOutFrame->mrfiPkt), F_HOP_COUNT, MAX_HOPS_FROM_AP);
    /* It's gonna be a forwarded frame. */
    PUT_INTO_FRAME(MRFI_P_PAYLOAD(&pOutFrame->mrfiPkt), F_FWD_FRAME, 0x80);

    nwk_sendFrame(pOutFrame, MRFI_TX_TYPE_FORCED);
  }
  else
  {
    nwk_SendEmptyPollRspFrame(frame);
  }

  return;
}

/******************************************************************************
 * @fn          nwk_resetSFMarker
 *
 * @brief       Reset S&F cklient marker so the TIDs resync.
 *
 * input parameters
 * @param  idx  - index of the client that should be reset.
 *
 * output parameters
 *
 * @return   void
 */
void nwk_resetSFMarker(uint8_t idx)
{
  sSFMarker[idx] = 0;

  return;
}

#else  /* ACCESS_POINT */

/******************************************************************************
 * @fn          nwk_poll
 *
 * @brief       Poll S&F server for any waiting frames.
 *
 * input parameters
 * @param  port  - Port on peer.
 * @param  addr  - SimpliciTI address of peer.
 *
 * output parameters
 *
 * @return   SMPL_SUCCESS
 *           SMPL_NO_AP_ADDRESS - We don't know Access Point's address
 *           SMPL_NOMEM         - no room in output frame queue
 *           SMPL_TX_CCA_FAIL   - CCA failure
 */
smplStatus_t nwk_poll(uint8_t port, uint8_t *addr)
{
  uint8_t        msg[MGMT_POLL_FRAME_SIZE];
  ioctlRawSend_t send;

  msg[MB_APP_INFO_OS] = MGMT_REQ_POLL;
  msg[MB_TID_OS]      = sTid;
  msg[M_POLL_PORT_OS] = port;
  memcpy(msg+M_POLL_ADDR_OS, addr, NET_ADDR_SIZE);

  /* it's OK to increment the TID here because the reply will not be
   * matched based on this number. The reply to the poll comes back
   * to the client port, not the Management port.
   */
  sTid++;

  if (!sAPAddr)
  {
    sAPAddr = nwk_getAPAddress();
    if (!sAPAddr)
    {
      return SMPL_NO_AP_ADDRESS;
    }
  }
  send.addr = (addr_t *)sAPAddr;
  send.msg  = msg;
  send.len  = sizeof(msg);
  send.port = SMPL_PORT_MGMT;

  return SMPL_Ioctl(IOCTL_OBJ_RAW_IO, IOCTL_ACT_WRITE, &send);
}

#endif /* ACCESS_POINT */
