/**************************************************************************************************
  Filename:       nwk_join.c
  Revised:        $Date: 2009-01-06 15:45:54 -0800 (Tue, 06 Jan 2009) $
  Revision:       $Revision: 18697 $

  Description:    This file supports the SimpliciTI Join network application.

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
#include "nwk_link.h"
#include "nwk_join.h"
#include "nwk_globals.h"
#include "nwk_freq.h"
#include "nwk_security.h"
#include "nwk_mgmt.h"

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

static          uint32_t sJoinToken = 0;
static          uint8_t (*spCallback)(linkID_t) = NULL;
static volatile uint8_t  sTid = 0;

#ifdef ACCESS_POINT
static sfInfo_t *spSandFContext = NULL;
static uint8_t   sJoinOK = 0;
#endif /* ACCESS_POINT */

/******************************************************************************
 * LOCAL FUNCTIONS
 */
#ifdef ACCESS_POINT
static void     smpl_send_join_reply(mrfiPacket_t *frame);
static uint32_t generateLinkToken(void);
static void     handleJoinRequest(mrfiPacket_t *);
#endif  /*  ACCESS_POINT */

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * GLOBAL FUNCTIONS
 */

/******************************************************************************
 * @fn          nwk_joinInit
 *
 * @brief       Initialize Join application.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   void
 */
void nwk_joinInit(uint8_t (*pf)(linkID_t))
{
  if (!sJoinToken) 
  {
    sJoinToken = DEFAULT_JOIN_TOKEN;
  }
  
  spCallback = pf;
  (void) spCallback;  /* keep compiler happy if we don't use this */

  sTid = MRFI_RandomByte() ;

#ifdef ACCESS_POINT
  /* set link token to something other than deafult if desired */
  nwk_setLinkToken(generateLinkToken());
#if defined(STARTUP_JOINCONTEXT_ON)
  sJoinOK = 1;
#elif defined(STARTUP_JOINCONTEXT_OFF)
  sJoinOK = 0;
#else
#error ERROR: Must define either STARTUP_JOINCONTEXT_ON or STARTUP_JOINCONTEXT_OFF
#endif
  spSandFContext = nwk_getSFInfoPtr();
#endif

  return;
}

/******************************************************************************
 * @fn          nwk_setJoinToken
 *
 * @brief       Sets the join token.
 *
 * input parameters
 * @param   token   - join token to be used on this network.
 *
 * output parameters
 *         no room in output queue.
 *
 * @return   void
 */
void nwk_setJoinToken(uint32_t token)
{
  /* only set if the supplied token is non-zero. */
  if (token)
  {
    sJoinToken = token;
  }

  return;
}

/******************************************************************************
 * @fn          nwk_getJoinToken
 *
 * @brief       Gets the current join token.
 *
 * input parameters
 *
 * output parameters
 * @param   pToken   - pointer to the returned value.
 *
 * @return   Current link token
 */
void nwk_getJoinToken(uint32_t *pToken)
{
  /* only set if the supplied token is non-zero. */
  if (pToken)
  {
    *pToken = sJoinToken;
  }

  return;
}

/******************************************************************************
 * @fn          generateLinkToken
 *
 * @brief       Generate the link token to be used for the network controlled
 *              by this Access Point.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   void
 */
#ifdef ACCESS_POINT
static uint32_t generateLinkToken(void)
{
  return 0xDEADBEEF;
}

/******************************************************************************
 * @fn          smpl_send_join_reply
 *
 * @brief       Send the Join reply. Include the Link token. If the device is
 *              a polling sleeper put it into the list of store-and-forward
 *              clients.
 *
 * input parameters
 * @param   frame     - join frame for which a reply is needed...maybe
 *
 * output parameters
 *
 * @return   void
 */
static void smpl_send_join_reply(mrfiPacket_t *frame)
{
  frameInfo_t *pOutFrame;
  uint8_t      msg[JOIN_REPLY_FRAME_SIZE];

  /* Is this a legacy frame? If so continue. Otherwise check verion.*/
  if ((MRFI_GET_PAYLOAD_LEN(frame) - F_APP_PAYLOAD_OS) > JOIN_LEGACY_MSG_LENGTH)
  {
    /* see if protocol version is correct... */
    if (*(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+J_PROTOCOL_VERSION_OS) != nwk_getProtocolVersion())
    {
      /* Accommodation of protocol version differences can be noted or accomplished here.
       * Otherwise, no match and the board goes back
       */
      return;
    }
  }


  /* see if join token is correct */
  {
    uint32_t jt;

    nwk_getNumObjectFromMsg(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+J_JOIN_TOKEN_OS, &jt, sizeof(jt));
    if (jt != sJoinToken)
    {
      return;
    }
  }

  /* send reply with tid, the link token, and the encryption context */
  {
    uint32_t linkToken;

    nwk_getLinkToken(&linkToken);
    nwk_putNumObjectIntoMsg((void *)&linkToken, msg+JR_LINK_TOKEN_OS, sizeof(linkToken));
  }
  msg[JR_CRYPTKEY_SIZE_OS] = SEC_CRYPT_KEY_SIZE;
  msg[JB_REQ_OS]           = JOIN_REQ_JOIN | NWK_APP_REPLY_BIT;
  /* sender's tid... */
  msg[JB_TID_OS]           = *(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+JB_TID_OS);

  if (pOutFrame = nwk_buildFrame(SMPL_PORT_JOIN, msg, sizeof(msg), MAX_HOPS_FROM_AP))
  {
    /* destination address is the source adddress of the received frame. */
    memcpy(MRFI_P_DST_ADDR(&pOutFrame->mrfiPkt), MRFI_P_SRC_ADDR(frame), NET_ADDR_SIZE);

#ifdef AP_IS_DATA_HUB
    /* if source device supports ED objects save source address to detect duplicate joins */
    if (*(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+J_NUMCONN_OS))
    {
      if (nwk_saveJoinedDevice(frame) && spCallback)
      {
        spCallback(0);
      }
    }
#endif
  }
  else
  {
    /* oops -- no room left for Tx frame. Don't send reply. */
    return;
  }

  /* If this device polls we need to provide store-and-forward support */
  if (GET_FROM_FRAME(MRFI_P_PAYLOAD(frame),F_RX_TYPE) == F_RX_TYPE_POLLS)
  {
    uint8_t loc;

    /* Check duplicate status */
    if (!nwk_isSandFClient(MRFI_P_SRC_ADDR(frame), &loc))
    {
      uint8_t        *pNumc   = &spSandFContext->curNumSFClients;
      sfClientInfo_t *pClient = &spSandFContext->sfClients[*pNumc];

      /* It's not a duplicate. Save it if there's room */
      if (*pNumc < NUM_STORE_AND_FWD_CLIENTS)
      {
        memcpy(pClient->clientAddr.addr, MRFI_P_SRC_ADDR(frame), NET_ADDR_SIZE);
        *pNumc = *pNumc + 1;
      }
      else
      {
        /* No room left. Just return and don't send reply. */
        return;
      }
    }
    else
    {
      /* We get here if it's a duplicate. We drop through and send reply.
       * Reset the S&F marker in the Management application -- we should
       * assume that the Client reset so the TID will be random. If this is
       * simply a duplicate frame it causes no harm.
       */
      nwk_resetSFMarker(loc);
    }
  }

#ifdef SMPL_SECURE
    nwk_setSecureFrame(&pOutFrame->mrfiPkt, sizeof(msg), 0);
#endif  /* SMPL_SECURE */

  /* It's not S&F or it is but we're OK to send reply. */
  nwk_sendFrame(pOutFrame, MRFI_TX_TYPE_FORCED);

  return;
}

/******************************************************************************
 * @fn          nwk_join
 *
 * @brief       Stub Join function for Access Points.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   Always returns SMPL_SUCCESS.
 */
smplStatus_t nwk_join(void)
{
  return SMPL_SUCCESS;
}

/******************************************************************************
 * @fn          nwk_isSandFClient
 *
 * @brief       Helper function to see if the destination of a frame we have is
 *              one of AP's store-and-forward clients.
 *
 * input parameters
 * @param   fPtr     - pointer to address in frame in question
 *
 * output parameters
 * @param   entLoc   - pointer to receive entry location in array of clients.
 *
 * @return   Returns client info structure pointer if the destination is a
 *           store-and-forward client, else 0.
 */
sfClientInfo_t *nwk_isSandFClient(uint8_t *pAddr, uint8_t *entLoc)
{
  uint8_t i;
  sfClientInfo_t *pSFClient = spSandFContext->sfClients;

  for (i=0; i<spSandFContext->curNumSFClients; ++i, ++pSFClient)
  {
    if (!memcmp(&pSFClient->clientAddr.addr, pAddr, NET_ADDR_SIZE))
    {
      *entLoc = i;
      return pSFClient;
    }
  }

  return 0;
}

/******************************************************************************
 * @fn          nwk_setJoinContext
 *
 * @brief       Helper function to set Join context for Access Point. This will
 *              allow arbitration bewteen potentially nearby Access Points when
 *              a new device is joining.
 *
 * input parameters
 * @param   which   - Join context is either off or on
 *
 * output parameters
 *
 * @return   void
 */
void nwk_setJoinContext(uint8_t which)
{
  sJoinOK = (JOIN_CONTEXT_ON == which) ? 1 : 0;

  return;
}

/******************************************************************************
 * @fn          handleJoinRequest
 *
 * @brief       Dispatches handler for specfic join request
 *
 * input parameters
 *
 * @param   frame - Join frame received
 *
 * output parameters
 *
 * @return   void
 */
static void handleJoinRequest(mrfiPacket_t *frame)
{
  if (JOIN_LEGACY_MSG_LENGTH == (MRFI_GET_PAYLOAD_LEN(frame) - F_APP_PAYLOAD_OS))
  {
    /* Legacy frame. Spoof a join request */
    *(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS) = JOIN_REQ_JOIN;
  }

  switch (*(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS))
  {
    case JOIN_REQ_JOIN:
      smpl_send_join_reply(frame);
      break;

    default:
      break;
  }

  return;
}

#else  /* ACCESS_POINT */

/******************************************************************************
 * @fn          nwk_join
 *
 * @brief       Join functioanlity for non-AP devices. Send the Join token
 *              and wait for the reply.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   Status of operation.
 */
smplStatus_t nwk_join(void)
{
  uint8_t      msg[JOIN_FRAME_SIZE];
  uint32_t     linkToken;
  addr_t       apAddr;
  uint8_t      radioState = MRFI_GetRadioState();
  smplStatus_t rc = SMPL_NO_JOIN;
  union
  {
    ioctlRawSend_t    send;
    ioctlRawReceive_t recv;
  } ioctl_info;

#if defined( FREQUENCY_AGILITY )
  uint8_t  i, numChan;
  freqEntry_t channels[NWK_FREQ_TBL_SIZE];

  if (!(numChan=nwk_scanForChannels(channels)))
  {
    return SMPL_NO_CHANNEL;
  }

  for (i=0; i<numChan; ++i)
  {
    nwk_setChannel(&channels[i]);
#else
  {
#endif

    ioctl_info.send.addr = (addr_t *)nwk_getBCastAddress();
    ioctl_info.send.msg  = msg;
    ioctl_info.send.len  = sizeof(msg);
    ioctl_info.send.port = SMPL_PORT_JOIN;

    /* Put join token in */
    nwk_putNumObjectIntoMsg((void *)&sJoinToken, msg+J_JOIN_TOKEN_OS, sizeof(sJoinToken));
    /* set app info byte */
    msg[JB_REQ_OS] = JOIN_REQ_JOIN;
    msg[JB_TID_OS] = sTid;
    /* Set number of connections supported. Used only by AP if it is
     * a data hub.
     */
    msg[J_NUMCONN_OS] = NUM_CONNECTIONS;
    /* protocol version number */
    msg[J_PROTOCOL_VERSION_OS] = nwk_getProtocolVersion();

    SMPL_Ioctl(IOCTL_OBJ_RAW_IO, IOCTL_ACT_WRITE, &ioctl_info.send);

    ioctl_info.recv.port = SMPL_PORT_JOIN;
    ioctl_info.recv.msg  = msg;
    ioctl_info.recv.addr = &apAddr;    /* save AP address from reply */

    NWK_CHECK_FOR_SETRX(radioState);
    NWK_REPLY_DELAY();
    NWK_CHECK_FOR_RESTORE_STATE(radioState);

    if (SMPL_SUCCESS == SMPL_Ioctl(IOCTL_OBJ_RAW_IO, IOCTL_ACT_READ, &ioctl_info.recv))
    {
      uint8_t firstByte = msg[JB_REQ_OS] & (~NWK_APP_REPLY_BIT);

      /* Sanity check for correct reply frame. Older version
       * has the length instead of the request as the first byte.
       */
      if ((firstByte == JOIN_REQ_JOIN) ||
          (firstByte == JOIN_REPLY_LEGACY_MSG_LENGTH)
         )
      {
        /* join reply returns link token */
        memcpy(&linkToken, msg+JR_LINK_TOKEN_OS, sizeof(linkToken));

        nwk_setLinkToken(linkToken);
        /* save AP address */
        nwk_setAPAddress(&apAddr);
        sTid++;   /* guard against duplicates */
        rc = SMPL_SUCCESS;
#if defined( FREQUENCY_AGILITY )
        break;
#endif
      }
    }
    /* TODO: process encryption stuff */
  }

  return rc;

}

#endif /* ACCESS_POINT */

/******************************************************************************
 * @fn          nwk_processJoin
 *
 * @brief       Processes a Join frame. If this is a reply let it go to the
 *              application. Otherwise generate and send the reply.
 *
 * input parameters
 * @param   frame     - Pointer to Join frame
 *
 * output parameters
 *
 * @return   Keep frame for application, release frame, or replay frame.
 */
fhStatus_t nwk_processJoin(mrfiPacket_t *frame)
{
  fhStatus_t rc = FHS_RELEASE;
  uint8_t    replyType;

  /* Make sure this is a reply and see if we sent this. Validate the
   * packet for reception by client app.
   */
  if (SMPL_MY_REPLY == (replyType=nwk_isValidReply(frame, sTid, JB_REQ_OS, JB_TID_OS)))
  {
    /* It's a match and it's a reply. Validate the received packet by
     * returning a 1 so it can be received by the client app.
     */
    MRFI_PostKillSem();
    rc = FHS_KEEP;
  }
#if defined(ACCESS_POINT)
  else if (SMPL_A_REPLY == replyType)
  {
    /* No match. If I'm not an ED this is a reply that should be passed on. */
    rc = FHS_REPLAY;
  }
  else
  {
    /* Send reply if we're an Access Point otherwise ignore the frame. */
    if ((SMPL_NOT_REPLY == replyType) && sJoinOK)
    {
      handleJoinRequest(frame);
    }
  }
#elif defined(RANGE_EXTENDER)
  else
  {
    /* Either a reply that has to be replayed or a request that
     * also must be replayed.
     */
    rc = FHS_REPLAY;
  }
#endif /* not END_DEVICE */

  (void) replyType;  /* keep compiler happy */

  return rc;
}
