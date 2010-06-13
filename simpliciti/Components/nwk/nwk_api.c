/**************************************************************************************************
  Filename:       nwk_api.c
  Revised:        $Date: 2009-01-28 18:27:38 -0800 (Wed, 28 Jan 2009) $
  Revision:       $Revision: 18875 $
  Author:         $Author: lfriedman $

  Description:    This file supports the SimpliciTI appliction layer API.

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
#include "nwk_app.h"
#include "mrfi.h"
#include "nwk_globals.h"
#include "nwk_freq.h"

/******************************************************************************
 * MACROS
 */

/******************************************************************************
 * CONSTANTS AND DEFINES
 */

/* These defines are in support an application listening for a link frame to
 * terminate after some amount of time. The intention is that this guard be
 * the exception. The intention of the SimpliciTI design is that the
 * temporal contiguity between the listen and the reception of the link frame
 * from the peer be very tight. The SMPL_LinkListen() should be termninated
 * by the reception of the link frame. But in case it does not receive the frame
 * the support below allows intervention by the application.
 */

/* The intention is for user to modify just the following single value */
#define LINKLISTEN_MILLISECONDS_2_WAIT    (5000)

#define LINKLISTEN_POLL_PERIOD_MS         (10)
#define LINKLISTEN_POLL_COUNT             ( (LINKLISTEN_MILLISECONDS_2_WAIT) / (LINKLISTEN_POLL_PERIOD_MS) )

/******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * LOCAL VARIABLES
 */
// [BM] Workaround to enable multiple stack restarts 
// static uint8_t sInit_done = 0;
uint8_t sInit_done = 0;

/******************************************************************************
 * LOCAL FUNCTIONS
 */
static uint8_t ioctlPreInitAccessIsOK(ioctlObject_t);

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * GLOBAL FUNCTIONS
 */

/***********************************************************************************
 * @fn          SMPL_Init
 *
 * @brief       Initialize the SimpliciTI stack.
 *
 * input parameters
 * @param   f  - Pointer to call back function. Function called by NWK when
 *               user application frame received. The callback is done in the
 *               ISR thread. Argument is Link ID associated with frame. Function
 *               returns 0 if frame is to be kept by NWK, otherwise 1. Frame
 *               should be kept if application will do a SMPL_Receive() in the
 *               user thread (recommended). Pointer may be NULL.
 *
 * output parameters
 *
 * @return   Status of operation:
 *             SMPL_SUCCESS
 *             SMPL_NO_JOIN     No Join reply. AP possibly not yet up.
 *             SMPL_NO_CHANNEL  Only if Frequency Agility enabled. Channel scan
 *                              failed. AP possibly not yet up.
 */
smplStatus_t SMPL_Init(uint8_t (*f)(linkID_t))
{
  smplStatus_t rc;

  if (!sInit_done)
  {
    /* set up radio. */
    MRFI_Init();

    /* initialize network */
    if ((rc=nwk_nwkInit(f)) != SMPL_SUCCESS)
    {
      return rc;
    }

    MRFI_WakeUp();
#if defined( FREQUENCY_AGILITY )
    {
      freqEntry_t chan;

      chan.logicalChan = 0;
      /* ok to set default channel explicitly now that MRFI initialized. */
      nwk_setChannel(&chan);
    }
#endif
    /* don't turn Rx on if we're an end device that isn't always on. */
#if !defined( END_DEVICE )
    MRFI_RxOn();
#endif

#if defined( END_DEVICE )
    /* All except End Devices are in promiscuous mode */
    MRFI_SetRxAddrFilter((uint8_t *)nwk_getMyAddress());
    MRFI_EnableRxAddrFilter();
#endif
  }
  sInit_done = 1;

  /* Join. if no AP or Join fails that status is returned. */
  rc = nwk_join();

  return rc;
}

/******************************************************************************
 * @fn          SMPL_LinkListen
 *
 * @brief       Listen for a link frame from a 'client' device.
 *
 * input parameters
 *
 * output parameters
 * @param   linkID     - pointer to Link ID to be used by application to
 *                       read and write to the linked peer.
 *
 * @return   status of operation.
 *             SMPL_SUCCESS
 *             SMPL_TIMEOUT  No link frame received during listen interval.
*                            Interval set in #defines above. linkID not valid.
 *
 */

smplStatus_t SMPL_LinkListen(linkID_t *linkID)
{
  uint8_t  radioState = MRFI_GetRadioState();
  uint16_t i;
  linkID_t locLinkID;

  /* Set the context. We want to reject any link frames received if
   * we're not listening. For example if we're an AP we are in
   * promiscuous mode and we'll see any broadcast link frames.
   */
  nwk_setListenContext(LINK_LISTEN_ON);

  NWK_CHECK_FOR_SETRX(radioState);

  for (i=0; i<LINKLISTEN_POLL_COUNT; ++i)
  {
    /* check the semaphore. local port is assigned when the reply is sent. */
    if ((locLinkID=nwk_getLocalLinkID()))
    {
      break;
    }
    NWK_DELAY(LINKLISTEN_POLL_PERIOD_MS);
  }

  NWK_CHECK_FOR_RESTORE_STATE(radioState);

  /* If the listen is terminated without hearing a message and setting a
   * link ID the listen context must be explicitly turned off.
   */
  if (!(locLinkID))
  {
    nwk_setListenContext(LINK_LISTEN_OFF);
    return SMPL_TIMEOUT;
  }

  *linkID = locLinkID;

  return SMPL_SUCCESS;
}

/******************************************************************************
 * @fn          SMPL_Send
 *
 * @brief       Send a message to a peer application. Old API kept for
 *              backward compatibility. Calls the new SMPL_SendOpt() with
 *              no options.
 *
 * input parameters
 * @param   lid     - Link ID (port) from application
 * @param   msg     - pointer to message from app to be sent
 * @param   len     - length of enclosed message
 *
 * output parameters
 *
 * @return   Status of operation. On a filaure the frame buffer is discarded
 *           and the Send call must be redone by the app.
 *             SMPL_SUCCESS
 *             SMPL_BAD_PARAM    No valid Connection Table entry for Link ID
 *                               Data in Connection Table entry bad
 *                               No message or message too long
 *             SMPL_NOMEM        No room in output frame queue
 *             SMPL_TX_CCA_FAIL  CCA failure.
 */
smplStatus_t SMPL_Send(linkID_t lid, uint8_t *msg, uint8_t len)
{
  return SMPL_SendOpt(lid, msg, len, SMPL_TXOPTION_NONE);
}

/******************************************************************************
 * @fn          SMPL_SendOpt
 *
 * @brief       Send a message to a peer application.
 *
 * input parameters
 * @param   lid     - Link ID (port) from application
 * @param   msg     - pointer to message from app to be sent
 * @param   len     - length of enclosed message
 * @param   options - Transmit options (bit map)
 *
 * output parameters
 *
 * @return   Status of operation. On a filaure the frame buffer is discarded
 *           and the Send call must be redone by the app.
 *             SMPL_SUCCESS
 *             SMPL_BAD_PARAM    No valid Connection Table entry for Link ID
 *                               Data in Connection Table entry bad
 *                               No message or message too long
 *             SMPL_NOMEM        No room in output frame queue
 *             SMPL_TX_CCA_FAIL  CCA failure.
 *             SMPL_NO_ACK       If application auto acknowledgement enabled
 *                               and no acknowledgement is received
 */
smplStatus_t SMPL_SendOpt(linkID_t lid, uint8_t *msg, uint8_t len, txOpt_t options)
{
  frameInfo_t  *pFrameInfo;
  connInfo_t   *pCInfo     = nwk_getConnInfo(lid);
  smplStatus_t  rc         = SMPL_BAD_PARAM;
  uint8_t       radioState = MRFI_GetRadioState();
  uint8_t       ackreq     = 0;
#if defined(ACCESS_POINT)
  uint8_t  loc;
#endif

  /* we have the connection info for this Link ID. make sure it is valid. */
   if (!pCInfo || ((rc=nwk_checkConnInfo(pCInfo, CHK_TX)) != SMPL_SUCCESS))
  {
    return rc;
  }

  /* parameter sanity check... */
  if (!msg || (len > MAX_APP_PAYLOAD))
  {
    return rc;
  }

  /* Build an outgoing message frame destined for the port from the
   * connection info using the destination address also from the
   * connection info.
   */
  if (SMPL_TXOPTION_NONE == options)
  {
    pFrameInfo = nwk_buildFrame(pCInfo->portTx, msg, len, pCInfo->hops2target);
  }
#if defined(APP_AUTO_ACK)
  else if (options & SMPL_TXOPTION_ACKREQ)
  {
    if (SMPL_LINKID_USER_UUD != lid)
    {
      pFrameInfo = nwk_buildAckReqFrame(pCInfo->portTx, msg, len, pCInfo->hops2target, &pCInfo->ackTID);
      ackreq     = 1;
    }
    else
    {
      /* can't request an ack on the UUD link ID */
      return SMPL_BAD_PARAM;
    }
  }
#endif  /* APP_AUTO_ACK */
  else
  {
    return SMPL_BAD_PARAM;
  }

  if (!pFrameInfo)
  {
    return SMPL_NOMEM;
  }
  memcpy(MRFI_P_DST_ADDR(&pFrameInfo->mrfiPkt), pCInfo->peerAddr, NET_ADDR_SIZE);

#if defined(SMPL_SECURE)
  {
    uint32_t *pUL = 0;

    if (pCInfo->thisLinkID != SMPL_LINKID_USER_UUD)
    {
      pUL = &pCInfo->connTxCTR;
    }
    nwk_setSecureFrame(&pFrameInfo->mrfiPkt, len, pUL);
  }
#endif  /* SMPL_SECURE */

#if defined(ACCESS_POINT)
  /* If we are an AP trying to send to a polling device, don't do it.
   * See if the target is a store-and-forward client.
   */
  if (nwk_isSandFClient(MRFI_P_DST_ADDR(&pFrameInfo->mrfiPkt), &loc))
  {
     pFrameInfo->fi_usage = FI_INUSE_UNTIL_FWD;
     return SMPL_SUCCESS;
  }
  else
#endif  /* ACCESS_POINT */
  {
    rc = nwk_sendFrame(pFrameInfo, MRFI_TX_TYPE_CCA);
  }

#if !defined(APP_AUTO_ACK)
  /* save a little code space with this #if */
  (void) ackreq;    /* keep compiler happy */
  return rc;
#else
  /* we're done if the send failed or no ack requested. */
  if (SMPL_SUCCESS != rc || !ackreq)
  {
    return rc;
  }

  NWK_CHECK_FOR_SETRX(radioState);
  NWK_REPLY_DELAY();
  NWK_CHECK_FOR_RESTORE_STATE(radioState);

  {
    bspIState_t intState;

    /* If the saved TID hasn't been reset then we never got the ack. */
    BSP_ENTER_CRITICAL_SECTION(intState);
    if (pCInfo->ackTID)
    {
      pCInfo->ackTID = 0;
      rc = SMPL_NO_ACK;
    }
    BSP_EXIT_CRITICAL_SECTION(intState);
  }

  return rc;
#endif  /* APP_AUTO_ACK */
}

/**************************************************************************************
 * @fn          SMPL_Receive
 *
 * @brief       Receive a message from a peer application.
 *
 * input parameters
 * @param   lid     - Link ID (port) from application
 *
 *
 * output parameters
 * @param   msg     - pointer to where received message should be copied.
 *                    buffer should be of size == MAX_APP_PAYLOAD
 * @param   len     - pointer to receive length of received message
 *
 * @return    Status of operation.
 *            Caller should not use the value returned in 'len' to decide
 *            whether there is a frame or not. It could be useful to the
 *            Caller to distinguish between no frame and a frame with no data.
 *            For example, in the polling case a frame with no application payload
 *            is the way the AP conveys that there are no frames waiting.
 *
 *              SMPL_SUCCESS
 *
 *              SMPL_BAD_PARAM  No valid Connection Table entry for Link ID
 *                              Data in Connection Table entry bad
 *              SMPL_NO_FRAME   No frame received.
 *              SMPL_NO_PAYLOAD Frame received with no payload (not necessarily
 *                              an error and could be deduced by application
 *                              because the returned length will be 0)
 *
 *            Polling device only:
 *
 *              SMPL_TIMEOUT        No response from Access Point
 *              SMPL_NO_AP_ADDRESS  Access Point address unknown
 *              SMPL_TX_CCA_FAIL    Could not send poll frame
 *              SMPL_NOMEM          No memory in output frame queue
 *              SMPL_NO_CHANNEL     Frequency Agility enabled and could not find channel
 */
smplStatus_t SMPL_Receive(linkID_t lid, uint8_t *msg, uint8_t *len)
{
  connInfo_t  *pCInfo = nwk_getConnInfo(lid);
  smplStatus_t rc = SMPL_BAD_PARAM;
  rcvContext_t rcv;

  if (!pCInfo || ((rc=nwk_checkConnInfo(pCInfo, CHK_RX)) != SMPL_SUCCESS))
  {
    return rc;
  }

  rcv.type  = RCV_APP_LID;
  rcv.t.lid = lid;

#if defined(RX_POLLS)
  {
    uint8_t numChans  = 1;
#if defined(FREQUENCY_AGILITY)
    freqEntry_t chans[NWK_FREQ_TBL_SIZE];
    uint8_t     scannedB4 = 0;
#endif

    do
    {
      uint8_t radioState = MRFI_GetRadioState();

      /* I'm polling. Do the poll to stimulate the sending of a frame. If the
       * frame has application length of 0 it means there were no frames.  If
       * no reply is received infer that the channel is changed. We then need
       * to scan and then retry the poll on each channel returned.
       */
      if (SMPL_SUCCESS != (rc=nwk_poll(pCInfo->portRx, pCInfo->peerAddr)))
      {
        /* for some reason couldn't send the poll out. */
        return rc;
      }

      /* do this before code block below which may reset it. */
      numChans--;

      /* Wait until there's a frame. if the len is 0 then return SMPL_NO_FRAME
       * to the caller. In the poll case the AP always sends something.
       */
      NWK_CHECK_FOR_SETRX(radioState);
      NWK_REPLY_DELAY();
      NWK_CHECK_FOR_RESTORE_STATE(radioState);

      /* TODO: deal with pending */
      rc = nwk_retrieveFrame(&rcv, msg, len, 0, 0);

#if defined(FREQUENCY_AGILITY)
      if (SMPL_SUCCESS == rc)
      {
        /* we received something... */
        return (*len) ? SMPL_SUCCESS : SMPL_NO_PAYLOAD;
      }

      /* No reply. scan for other channel(s) if we haven't already. Then set
       * one and try again.
       */
      if (!scannedB4)
      {
        numChans  = nwk_scanForChannels(chans);
        scannedB4 = 1;
      }
      if (numChans)
      {
        nwk_setChannel(&chans[numChans-1]);
      }
#else /*  FREQUENCY_AGILITY */
      return (*len) ? rc : ((SMPL_SUCCESS == rc) ? SMPL_NO_PAYLOAD : SMPL_TIMEOUT);
#endif
    } while (numChans);
  }

#if defined(FREQUENCY_AGILITY)
  return SMPL_NO_CHANNEL;
#endif

#else  /* RX_POLLS */
  return nwk_retrieveFrame(&rcv, msg, len, 0, 0);
#endif  /* RX_POLLS */
}


/******************************************************************************
 * @fn          SMPL_Link
 *
 * @brief       Link to a peer.
 *
 * input parameters
 *
 * output parameters
 * @param   lid     - pointer to where we should write the link ID to which the
 *                    application will read and write.
 *
 * @return   Status of operation.
 *             SMPL_SUCCESS
 *             SMPL_NOMEM         No room to allocate local Rx port, no more
 *                                room in Connection Table, or no room in
 *                                output frame queue.
 *             SMPL_NO_LINK       No reply frame during wait window.
 *             SMPL_TX_CCA_FAIL   Could not send Link frame.
 */
smplStatus_t SMPL_Link(linkID_t *lid)
{
  return nwk_link(lid);
}

#if defined(EXTENDED_API)
/**************************************************************************************
 * @fn          SMPL_Unlink
 *
 * @brief       Tear down connection to a peer.
 *
 * input parameters
 * @param   lid     - Link ID whose connection is to be terminated.
 *
 * output parameters
 *
 * @return   Status of operation. The Connection Table entry for the Link ID
 *           is always freed successfuly. The returned status value is the
 *           status of the _peer's_ connection tear-down as a result of the
 *           message sent here.
 *           SMPL_SUCCESS         Local and remote connection destroyed.
 *           SMPL_BAD_PARAM       No local connection table entry for this Link ID
 *           SMPL_TIMEOUT         No reply from peer.
 *           SMPL_NO_PEER_UNLINK  Peer did not have a Connection Table entry for me.
 */
smplStatus_t SMPL_Unlink(linkID_t lid)
{
  return nwk_unlink(lid);
}

/**************************************************************************************
 * @fn          SMPL_Ping
 *
 * @brief       Ping a peer. Synchronous call. Although a link ID is used it is the
 *              NWK Ping application that is pinged, not the peer of this Link ID. The
 *              peer is not expected to be the responder to the frame sent from here.
 *              This API is a proxy for a real ping since the application doesn't
 *              have direct access to SimpliciTI device addresses. Kind of hokey but a
 *              useful keep-alive mechanism without having to support it with
 *              user application service.
 *
 * input parameters
 * @param   lid  - The link ID whose peer device address is used to direct the NWK Ping
 *
 * output parameters
 *
 * @return   Status of operation.
 */
smplStatus_t SMPL_Ping(linkID_t lid)
{
  return nwk_ping(lid);
}

/**************************************************************************************
 * @fn          SMPL_Commission
 *
 * @brief       Commission a connection.
 *
 * input parameters
 * @param   peerAddr  - Pointer to address of the peer for this connection
 * @param    locPort  - Port on which to listen for messages from the peer
 * @param    rmtPort  - Port on which to send messages to the peer.
 * @param        lid  - Pointer to Link ID object. If content of location is
 *                      non-zero on input the value is placed in the Connection
 *                      object.
 *
 * output parameters
 * @param        lid  - Pointer to Link ID object. If content of location is zero
 *                      on input the value in the Connection object is stored there.
 *
 * @return   SMPL_SUCCESS
 *           SMPL_NOMEM     - No room left in Connection table.
 *           SMPL_BAD_PARAM - A pointer to a Link object was not supplied.
 */
smplStatus_t SMPL_Commission(addr_t *peerAddr, uint8_t locPort, uint8_t rmtPort, linkID_t *lid)
{
  connInfo_t   *pCInfo = nwk_getNextConnection();
  smplStatus_t  rc     = SMPL_BAD_PARAM;

  do {
    if (pCInfo)
    {
      /* sanity checks... */

      /* Check port info. */
      if ((locPort > SMPL_PORT_STATIC_MAX) || (locPort < (SMPL_PORT_STATIC_MAX - PORT_USER_STATIC_NUM + 1)))
      {
        continue;
      }

      if ((rmtPort > SMPL_PORT_STATIC_MAX) || (rmtPort < (SMPL_PORT_STATIC_MAX - PORT_USER_STATIC_NUM + 1)))
      {
        continue;
      }

      /* Must supply a pointer to the Link ID object */
      if (!lid)
      {
        /* No Link ID pointer supplied */
        continue;
      }

      /* we're sane */

      /* Use the value generated at connection object assign time. */
      *lid = pCInfo->thisLinkID;

      /* store peer's address */
      memcpy(pCInfo->peerAddr, peerAddr, NET_ADDR_SIZE);

      /* store port info */
      pCInfo->portRx = locPort;
      pCInfo->portTx = rmtPort;

      pCInfo->hops2target = MAX_HOPS;

      rc = SMPL_SUCCESS;
    }
    else
    {
      /* No room in Connection table */
      rc = SMPL_NOMEM;
    }
  } while (0);

  if ((SMPL_SUCCESS != rc) && pCInfo)
  {
    nwk_freeConnection(pCInfo);
  }

  return rc;
}
#endif   /* EXTENDED_API */

/******************************************************************************
 * @fn          SMPL_Ioctl
 *
 * @brief       This routine supplies the SimpliciTI IOCTL support.
 *
 * input parameters
 * @param   object   - The IOCTL target object
 * @param   action   - The IOCTL target action on the object
 * @param   val      - pointer to value. exact forn depends on object type.
 *
 * output parameters
 *
 * @return   Status of action. Value depends on object, action, and result.
 *
 *           SMPL_BAD_PARAM is returned if this API is called before
 *                          initialization and the object is not one of
 *                          the valid exceptions.
 */
smplStatus_t SMPL_Ioctl(ioctlObject_t object, ioctlAction_t action, void *val)
{
  smplStatus_t rc;

  /* if init hasn't occurred see if access is still valid */
  if (!sInit_done && !ioctlPreInitAccessIsOK(object))
  {
    return SMPL_BAD_PARAM;
  }

  switch (object)
  {
#if defined(EXTENDED_API)
    case IOCTL_OBJ_TOKEN:
      {
        ioctlToken_t *t = (ioctlToken_t *)val;

        rc = SMPL_SUCCESS;
        if (TT_LINK == t->tokenType)
        {
          if (IOCTL_ACT_SET == action)
          {
            nwk_setLinkToken(t->token.linkToken);
          }
          else if (IOCTL_ACT_GET == action)
          {
            nwk_getLinkToken(&t->token.linkToken);
          }
          else
          {
            rc = SMPL_BAD_PARAM;
          }
        }
        else if (TT_JOIN == t->tokenType)
        {
          if (IOCTL_ACT_SET == action)
          {
            nwk_setJoinToken(t->token.joinToken);
          }
          else if (IOCTL_ACT_GET == action)
          {
            nwk_getJoinToken(&t->token.joinToken);
          }
          else
          {
            rc = SMPL_BAD_PARAM;
          }
        }
        else
        {
          rc = SMPL_BAD_PARAM;
        }
      }
      break;

    case IOCTL_OBJ_NVOBJ:
      rc = nwk_NVObj(action, (ioctlNVObj_t *)val);
      break;
#endif  /* EXTENDED_API */

    case IOCTL_OBJ_CONNOBJ:
      rc = nwk_connectionControl(action, val);
      break;

    case IOCTL_OBJ_ADDR:
      if ((IOCTL_ACT_GET == action) || (IOCTL_ACT_SET == action))
      {
        rc = nwk_deviceAddress(action, (addr_t *)val);
      }
      else
      {
        rc = SMPL_BAD_PARAM;
      }
      break;

    case IOCTL_OBJ_RAW_IO:
      if (IOCTL_ACT_WRITE == action)
      {
        rc = nwk_rawSend((ioctlRawSend_t *)val);
      }
      else if (IOCTL_ACT_READ == action)
      {
        rc = nwk_rawReceive((ioctlRawReceive_t *)val);
      }
      else
      {
        rc = SMPL_BAD_PARAM;
      }
      break;

    case IOCTL_OBJ_RADIO:
      rc = nwk_radioControl(action, val);
      break;

#if defined(ACCESS_POINT)
    case IOCTL_OBJ_AP_JOIN:
      rc = nwk_joinContext(action);
      break;
#endif
#if defined(FREQUENCY_AGILITY)
    case IOCTL_OBJ_FREQ:
      rc = nwk_freqControl(action, val);
      break;
#endif
    case IOCTL_OBJ_FWVER:
      if (IOCTL_ACT_GET == action)
      {
        memcpy(val, nwk_getFWVersion(), SMPL_FWVERSION_SIZE);
        rc = SMPL_SUCCESS;
      }
      else
      {
        rc = SMPL_BAD_PARAM;
      }
      break;

    case IOCTL_OBJ_PROTOVER:
      if (IOCTL_ACT_GET == action)
      {
        *((uint8_t *)val) = nwk_getProtocolVersion();
        rc = SMPL_SUCCESS;
      }
      else
      {
        rc = SMPL_BAD_PARAM;
      }
      break;

    default:
      rc = SMPL_BAD_PARAM;
      break;
  }

  return rc;
}

/******************************************************************************
 * @fn          ioctlPreInitAccessIsOK
 *
 * @brief       Is the request legal yet? Most requests are not legal before
 *              SMPL_Init().
 *
 * input parameters
 * @param   object   - The IOCTL target object
 *
 * output parameters
 *
 * @return   Returns non-zero if request should be honored for further
 *           processing, otherwise returns 0. This function does not
 *           determine of the object-action pair are valid. It only knows
 *           about exceptions, i.e., those that are valid before the
 *           SMPL_Init() call.
 *
 */
static uint8_t ioctlPreInitAccessIsOK(ioctlObject_t object)
{
  uint8_t rc;

  /* Currently the only legal pre-init accesses are the address and
   * the token objects.
   */
  switch (object)
  {
    case IOCTL_OBJ_ADDR:
    case IOCTL_OBJ_TOKEN:
      rc = 1;   /* legal */
      break;

    default:
      rc = 0;   /* not legal when init not done */
      break;
  }

  return rc;
}
