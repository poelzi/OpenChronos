/**************************************************************************************************
  Filename:       nwk_frame.c
  Revised:        $Date: 2009-03-10 16:21:40 -0700 (Tue, 10 Mar 2009) $
  Revision:       $Revision: 19368 $
  Author          $Author: lfriedman $

  Description:    This file supports the SimpliciTI frame handling functions.

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
#include "nwk_frame.h"
#include "nwk.h"
#include "nwk_app.h"
#include "nwk_QMgmt.h"
#include "nwk_globals.h"
#include "nwk_mgmt.h"
#include "nwk_security.h"

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

#if SIZE_INFRAME_Q > 0
/* array of function pointers to handle NWK application frames */
static  fhStatus_t (* const func[])(mrfiPacket_t *) = { nwk_processPing,
                                                        nwk_processLink,
                                                        nwk_processJoin,
                                                        nwk_processSecurity,
                                                        nwk_processFreq,
                                                        nwk_processMgmt
                                                      };
#endif  /* SIZE_INFRAME_Q > 0 */

static uint8_t sTRACTID = 0;

static addr_t const *sMyAddr = NULL;

static uint8_t  sMyRxType = 0, sMyTxType = 0;

#if !defined(RX_POLLS)
static uint8_t  (*spCallback)(linkID_t) = NULL;
#endif

/******************************************************************************
 * LOCAL FUNCTIONS
 */

#if SIZE_INFRAME_Q > 0
/* local helper functions for Rx devices */
static void  dispatchFrame(frameInfo_t *);
#if !defined(END_DEVICE)
#if defined(ACCESS_POINT)
/* only Access Points need to worry about duplicate S&F frames */
uint8_t  isDupSandFFrame(mrfiPacket_t *);
#endif /* ACCESS_POINT */
#endif  /* !END_DEVICE */
#endif  /* SIZE_INFRAME_Q > 0 */

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * GLOBAL FUNCTIONS
 */

/******************************************************************************
 * @fn          nwk_frameInit
 *
 * @brief       Initialize network context.
 *
 * input parameters
 *       pF - Pointer to callback function. If none intended should be NULL.
 *
 * output parameters
 *
 * @return    void
 */

void nwk_frameInit(uint8_t (*pF)(linkID_t))
{

/****** Fill static values for the DEVICEINFO byte that will go in each frame ******/
  /* Rx type when frame originates from this device. Set in nwk_buildFrame() */
  /* Tx type when frame sent from this device. Set in nwk_sendframe() */
#if !defined(END_DEVICE)
    sMyRxType = F_RX_TYPE_USER_CTL;
  #if defined(ACCESS_POINT)
    sMyTxType = F_TX_DEVICE_AP;
  #else
    sMyTxType = F_TX_DEVICE_RE;
  #endif
#else
    sMyTxType = F_TX_DEVICE_ED;
  #if defined(RX_POLLS)
    sMyRxType = F_RX_TYPE_POLLS;
  #endif
  #if defined(RX_USER)
    sMyRxType = F_RX_TYPE_USER_CTL;
  #endif
#endif
/****** DONE fill static values for the DEVICEINFO byte that will go in each frame ******/

#if !defined(RX_POLLS)
  spCallback = pF;
#else
  (void) pF;
#endif

  sMyAddr = nwk_getMyAddress();

  while (!(sTRACTID=MRFI_RandomByte())) ;

  return;
}

/******************************************************************************
 * @fn          nwk_buildFrame
 *
 * @brief       Builds an output frame for the port and message enclosed.
 *              This routine prepends the frame header and populates the
 *              frame in the output queue.
 *
 * input parameters
 * @param   port    - port from application
 * @param   msg     - pointer to message from app to be sent
 * @param   len     - length of enclosed message
 * @param   hops    - number of hops allowed. this is less than MAX_HOPS
 *                    whenever the frame is being sent to the AP. this is to
 *                    help mitigate the (short) broadcast storms
 *
 * output parameters
 *
 * @return   pointer to frameInfo_t structure created. NULL if there is
 *           no room in output queue.
 */
frameInfo_t *nwk_buildFrame(uint8_t port, uint8_t *msg, uint8_t len, uint8_t hops)
{
  frameInfo_t  *fInfoPtr;

  if (!(fInfoPtr=nwk_QfindSlot(OUTQ)))
  {
    return (frameInfo_t *)NULL;
  }

  MRFI_SET_PAYLOAD_LEN(&fInfoPtr->mrfiPkt, len+F_APP_PAYLOAD_OS);

  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_ENCRYPT_OS, 0);
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_PORT_OS, port);
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_TRACTID_OS, sTRACTID);
  while (!(++sTRACTID)) ;  /* transaction ID can't be 0 */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_RX_TYPE, sMyRxType);
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_HOP_COUNT, hops);

  /* reset ack-relevant bits */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_ACK_REQ, 0);
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_ACK_RPLY, 0);

  /* reset forwarding bit */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_FWD_FRAME, 0);

  memcpy(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt)+F_APP_PAYLOAD_OS, msg, len);
  memcpy(MRFI_P_SRC_ADDR(&fInfoPtr->mrfiPkt), sMyAddr, NET_ADDR_SIZE);

  return fInfoPtr;
}

#if defined(APP_AUTO_ACK)
/******************************************************************************
 * @fn          nwk_buildAckReqFrame
 *
 * @brief       Builds an output frame for the port and message enclosed.
 *              This routine prepends the frame header and populates the
 *              frame in the output queue. The frame is set to request that
 *              an ack frame be sent by the peer.
 *
 * input parameters
 * @param   port    - port from application
 * @param   msg     - pointer to message from app to be sent
 * @param   len     - length of enclosed message
 * @param   hops    - number of hops allowed. this is less than MAX_HOPS
 *                    whenever the frame is being sent to the AP. this is to
 *                    help mitigate the (short) broadcast storms
 * @param   tid     - Transaction ID to insert in NWK header used to match
 *                    the ack reply.
 *
 * output parameters
 *
 * @return   pointer to frameInfo_t structure created. NULL if there is
 *           no room in output queue.
 */
frameInfo_t *nwk_buildAckReqFrame(uint8_t port, uint8_t *msg, uint8_t len, uint8_t hops, volatile uint8_t *tid)
{
  frameInfo_t *fInfoPtr;

  /* Build a normal frame first. */
  if (!(fInfoPtr=nwk_buildFrame(port, msg, len, hops)))
  {
    return (frameInfo_t *)NULL;
  }

  /* save TID  */
  *tid = GET_FROM_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_TRACTID_OS);
  /* Set REQ_ACK bit */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fInfoPtr->mrfiPkt), F_ACK_REQ, F_ACK_REQ_TYPE);

  return fInfoPtr;
}
#endif  /* APP_AUTO_ACK */

#if SIZE_INFRAME_Q > 0
/******************************************************************************
 * @fn          MRFI_RxCompleteISR
 *
 * @brief       Here on Rx interrupt from radio. Process received frame from the
 *              radio Rx FIFO.
 *
 * input parameters
 *
 * output parameters
 *
 * @return      void
 */
void MRFI_RxCompleteISR()
{
  frameInfo_t  *fInfoPtr;

  /* room for more? */
  if (fInfoPtr=nwk_QfindSlot(INQ))
  {
    MRFI_Receive(&fInfoPtr->mrfiPkt);

    dispatchFrame(fInfoPtr);
  }

  return;
}

/******************************************************************************
 * @fn          nwk_retrieveFrame
 *
 * @brief       Retrieve frame from Rx frame queue. Invoked by application-level
 *              code either app through SMPL_Receive() or IOCTL through raw Rx. This
 *              should run in a user thread, not an ISR thread.
 *
 * input parameters
 * @param    port    - port on which to get a frame
 *
 * output parameters
 * @param    msg     - pointer to where app payload should be copied. Buffer
 *                     allocated should be == MAX_APP_PAYLOAD.
 *
 * @param    len      - pointer to where payload length should be stored. Caller
 *                      can check for non-zero when polling the port. initialized
 *                      to 0 even if no frame is retrieved.
 * @param    srcAddr  - if non-NULL, a pointer to where to copy the source address
 *                      of the retrieved message.
 * @param    hopCount - if non-NULL, a pointer to where to copy the hop count
                        of the retrieved message.
 *
 * @return    SMPL_SUCCESS
 *            SMPL_NO_FRAME  - no frame found for specified destination
 *            SMPL_BAD_PARAM - no valid connection info for the Link ID
 *
 */
smplStatus_t nwk_retrieveFrame(rcvContext_t *rcv, uint8_t *msg, uint8_t *len, addr_t *srcAddr, uint8_t *hopCount)
{
  frameInfo_t *fPtr;
  uint8_t      done;

  do {
    /* look for a frame on requested port. */
    *len = 0;
    done = 1;

    fPtr = nwk_QfindOldest(INQ, rcv, USAGE_NORMAL);
    if (fPtr)
    {
      connInfo_t  *pCInfo = 0;

      if (RCV_APP_LID == rcv->type)
      {
        pCInfo = nwk_getConnInfo(rcv->t.lid);
        if (!pCInfo)
        {
          return SMPL_BAD_PARAM;
        }
#if defined(SMPL_SECURE)
        /* decrypt here...we have all the context we need. */
        {
          uint32_t  ctr  = pCInfo->connRxCTR;
          uint32_t *pctr = &ctr;
          uint8_t   len  = MRFI_GET_PAYLOAD_LEN(&fPtr->mrfiPkt) - F_SEC_CTR_OS;

          if (pCInfo->thisLinkID == SMPL_LINKID_USER_UUD)
          {
            pctr = NULL;
          }
#if defined(RX_POLLS)
          else if ((F_APP_PAYLOAD_OS - F_SEC_CTR_OS) == len)
          {
            /* This was an empty poll reply frame generated by the AP.
             * It uses the single-byte CTR value like network applications.
             * We do not want to use the application layer counter in this case.
             */
            pctr = NULL;
          }
#endif
          if (nwk_getSecureFrame(&fPtr->mrfiPkt, len, pctr))
          {
            if (pctr)
            {
              /* Update connection's counter. */
              pCInfo->connRxCTR = ctr;
            }
          }
          else
          {
            /* Frame bogus. Check for another frame. */
            done = 0;
            continue;
          }
        }
#endif  /* SMPL_SECURE */
      }

      /* it's on the requested port. */
      *len = MRFI_GET_PAYLOAD_LEN(&fPtr->mrfiPkt) - F_APP_PAYLOAD_OS;
      memcpy(msg, MRFI_P_PAYLOAD(&fPtr->mrfiPkt)+F_APP_PAYLOAD_OS, *len);
      /* save signal info */
      if (pCInfo)
      {
        /* Save Rx metrics... */
        pCInfo->sigInfo.rssi = fPtr->mrfiPkt.rxMetrics[MRFI_RX_METRICS_RSSI_OFS];
        pCInfo->sigInfo.lqi  = fPtr->mrfiPkt.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS];
      }
      if (srcAddr)
      {
        /* copy source address if requested */
        memcpy(srcAddr, MRFI_P_SRC_ADDR(&fPtr->mrfiPkt), NET_ADDR_SIZE);
      }
      if (hopCount)
      {
        /* copy hop count if requested */
        *hopCount = GET_FROM_FRAME(MRFI_P_PAYLOAD(&fPtr->mrfiPkt), F_HOP_COUNT);
      }
      /* input frame no longer needed. free it. */
      nwk_QadjustOrder(INQ, fPtr->orderStamp);

      fPtr->fi_usage = FI_AVAILABLE;
      return SMPL_SUCCESS;
    }
  } while (!done);

  return SMPL_NO_FRAME;
}

/******************************************************************************
 * @fn          dispatchFrame
 *
 * @brief       Received frame looks OK so far. Dispatch to either NWK app by
 *              invoking the handler or the user's app by simply leaving the
 *              frame in the queue and letting the app poll the port.
 *
 * input parameters
 * @param   fiPtr    - frameInfo_t pointer to received frame
 *
 * output parameters
 *
 * @return   void
 */
static void dispatchFrame(frameInfo_t *fiPtr)
{
  uint8_t     port       = GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_PORT_OS);
  uint8_t     nwkAppSize = sizeof(func)/sizeof(func[0]);
  fhStatus_t  rc;
  linkID_t    lid;
#if defined(ACCESS_POINT)
  uint8_t loc;
#endif
#if !defined(END_DEVICE)
  uint8_t isForMe;
#endif

  /* be sure it's not an echo... */
  if (!memcmp(MRFI_P_SRC_ADDR(&fiPtr->mrfiPkt), sMyAddr, NET_ADDR_SIZE))
  {
    fiPtr->fi_usage = FI_AVAILABLE;
    return;
  }

  /* Make sure encyrption bit conforms to our security support context. */
#if defined(SMPL_SECURE)
  if (!(GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_ENCRYPT_OS)))
  {
    /* Encyrption bit is not on when when it should be */
    fiPtr->fi_usage = FI_AVAILABLE;
    return;
  }
#else
  if (GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_ENCRYPT_OS))
  {
    /* Encyrption bit is on when when it should not be */
    fiPtr->fi_usage = FI_AVAILABLE;
    return;
  }
#endif  /* SMPL_SECURE */

  /* If it's a network application port dispatch to service routine. Dispose
   * of frame depending on return code.
   */
  if (port && (port <= nwkAppSize))
  {
#if defined(SMPL_SECURE)
    /* Non-connection-based frame. We can decode here if it was encrypted */
    if (!nwk_getSecureFrame(&fiPtr->mrfiPkt, MRFI_GET_PAYLOAD_LEN(&fiPtr->mrfiPkt) - F_SEC_CTR_OS, 0))
    {
      fiPtr->fi_usage = FI_AVAILABLE;
      return;
    }
#endif
    rc = func[port-1](&fiPtr->mrfiPkt);
    if (FHS_KEEP == rc)
    {
      fiPtr->fi_usage = FI_INUSE_UNTIL_DEL;
    }
#if !defined(END_DEVICE)
    else if (FHS_REPLAY == rc)
    {
      /* an AP or an RE could be relaying a NWK application frame... */
      nwk_replayFrame(fiPtr);
    }
#endif
    else  /* rc == FHS_RELEASE (default...) */
    {
      fiPtr->fi_usage = FI_AVAILABLE;
    }
    return;
  }
  /* sanity check */
  else if ((port != SMPL_PORT_USER_BCAST) && ((port < PORT_BASE_NUMBER) || (port > SMPL_PORT_STATIC_MAX)))
  {
    /* bogus port. drop frame */
    fiPtr->fi_usage = FI_AVAILABLE;
    return;
  }

  /* At this point we know the target is a user app. If this is an end device
   * and we got this far save the frame and we're done. If we're an AP there
   * are 3 cases: it's for us, it's for s store-and-forward client, or we need
   * to replay the frame. If we're and RE and the frame didn't come from an RE
   * and it's not for us, replay the frame.
   */

#if defined(END_DEVICE)
  /* If we're s polling end device we only accept application frames from
   * the AP. This prevents duplicate reception if we happen to be on when
   * a linked peer sends.
   */
#if defined(RX_POLLS)
  if (F_TX_DEVICE_ED != GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_TX_DEVICE))
  {
    if (nwk_isConnectionValid(&fiPtr->mrfiPkt, &lid))
    {
      fiPtr->fi_usage = FI_INUSE_UNTIL_DEL;
    }
    else
    {
      fiPtr->fi_usage = FI_AVAILABLE;
    }
  }
  else
  {
    fiPtr->fi_usage = FI_AVAILABLE;
  }
#else
  /* it's destined for a user app. */
  if (nwk_isConnectionValid(&fiPtr->mrfiPkt, &lid))
  {
    fiPtr->fi_usage = FI_INUSE_UNTIL_DEL;
    if (spCallback && spCallback(lid))
    {
      fiPtr->fi_usage = FI_AVAILABLE;
      return;
    }
  }
  else
  {
    fiPtr->fi_usage = FI_AVAILABLE;
  }
#endif  /* RX_POLLS */

#else   /* END_DEVICE */

  /* We have an issue if the frame is broadcast to the UUD port. The AP (or RE) must
   * handle this frame as if it were the target in case there is an application
   * running that is listening on that port. But if it's a broadcast it must also be
   * replayed. It isn't enough just to test for the UUD port because it could be a
   * directed frame to another device. We must check explicitly for broadcast
   * destination address.
   */
  isForMe = !memcmp(sMyAddr, MRFI_P_DST_ADDR(&fiPtr->mrfiPkt), NET_ADDR_SIZE);
  if (isForMe || ((port == SMPL_PORT_USER_BCAST) && !memcmp(nwk_getBCastAddress(), MRFI_P_DST_ADDR(&fiPtr->mrfiPkt), NET_ADDR_SIZE)))
  {
    /* The folllowing test will succeed for the UUD port regardless of the
     * source address.
     */
    if (nwk_isConnectionValid(&fiPtr->mrfiPkt, &lid))
    {
      /* If this is for the UUD port and we are here then the device is either
       * an AP or an RE. In either case it must replay the UUD port frame if the
       * frame is not "for me". But it also must handle it since it could have a
       * UUD-listening application. Do the reply first and let the subsequent code
       * correctly set the frame usage state. Note that the routine return can be
       * from this code block. If not it will drop through to the bottom without
       * doing a replay.
       */
      /* Do I need to replay it? */
      if (!isForMe)
      {
        /* must be a broadcast for the UUD port */
        nwk_replayFrame(fiPtr);
      }
      /* OK. Now I handle it... */
      fiPtr->fi_usage = FI_INUSE_UNTIL_DEL;
      if (spCallback && spCallback(lid))
      {
        fiPtr->fi_usage = FI_AVAILABLE;
        return;
      }
    }
    else
    {
      fiPtr->fi_usage = FI_AVAILABLE;
    }
  }
#if defined( ACCESS_POINT )
  /* Check to see if we need to save this for a S and F client. Otherwise,
   * if it's not for us, get rid of it.
   */
  else if (nwk_isSandFClient(MRFI_P_DST_ADDR(&fiPtr->mrfiPkt), &loc))
  {
    /* Don't bother if it is a duplicate frame or if it's a forwarded frame
     * echoed back from an RE.
     */
    if (!isDupSandFFrame(&fiPtr->mrfiPkt) &&
        !(GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_FWD_FRAME))
       )
    {
#if defined(APP_AUTO_ACK)
      /* Make sure ack request bit is off. Sender will have gone away. */
      PUT_INTO_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_ACK_REQ, 0);
#endif
      fiPtr->fi_usage = FI_INUSE_UNTIL_FWD;
    }
    else
    {
      fiPtr->fi_usage = FI_AVAILABLE;
    }
  }
  else if (GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_TX_DEVICE) == F_TX_DEVICE_AP)
  {
    /* I'm an AP and this frame came from an AP. Don't replay. */
    fiPtr->fi_usage = FI_AVAILABLE;
  }
#elif defined( RANGE_EXTENDER )
  else if (GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_TX_DEVICE) == F_TX_DEVICE_RE)
  {
    /* I'm an RE and this frame came from an RE. Don't replay. */
    fiPtr->fi_usage = FI_AVAILABLE;
  }
#endif
  else
  {
    /* It's not for me and I'm either an AP or I'm an RE and the frame
     * didn't come from an RE. Replay the frame.
     */
    nwk_replayFrame(fiPtr);
  }
#endif  /* !END_DEVICE */
  return;
}
#endif   /* SIZE_INFRAME_Q > 0 */

/******************************************************************************
 * @fn          nwk_sendFrame
 *
 * @brief       Send a frame by copying it to the radio Tx FIFO.
 *
 * input parameters
 * @param   pFrameInfo   - pointer to frame to be sent
 * @param   txOption     - do CCA or force frame out.
 *
 * output parameters
 *
 * @return    SMPL_SUCCESS
 *            SMPL_TX_CCA_FAIL Tx failed because of CCA failure.
 *                             Tx FIFO flushed in this case.
 */
smplStatus_t nwk_sendFrame(frameInfo_t *pFrameInfo, uint8_t txOption)
{
  smplStatus_t rc;

  /* set the type of device sending the frame in the header */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&pFrameInfo->mrfiPkt), F_TX_DEVICE, sMyTxType);

  if (MRFI_TX_RESULT_SUCCESS == MRFI_Transmit(&pFrameInfo->mrfiPkt, txOption))
  {
    rc = SMPL_SUCCESS;
  }
  else
  {
    /* Tx failed -- probably CCA. free up frame buffer. We do not have NWK
     * level retries. Let application do it.
     */
    rc = SMPL_TX_CCA_FAIL;
  }

  /* TX is done. free up the frame buffer */
  pFrameInfo->fi_usage = FI_AVAILABLE;

  return rc;
}


/******************************************************************************
 * @fn          nwk_getMyRxType
 *
 * @brief       Get my Rx type. Used to help populate the hops count in the
 *              frame header to try and limit the broadcast storm. Info is
 *              exchanged when linking.
 *
 * input parameters
 *
 * output parameters
 *
 * @return      The address LSB.
 */
uint8_t nwk_getMyRxType(void)
{
  return sMyRxType;
}

#if defined(APP_AUTO_ACK)
/******************************************************************************
 * @fn          nwk_sendAckReply
 *
 * @brief       Send an acknowledgement reply frame.
 *
 * input parameters
 * @param   frame   - pointer to frame with ack request.
 * @param   port    - port on whcih reply expected.
 *
 * output parameters
 *
 * @return      void
 */
void nwk_sendAckReply(mrfiPacket_t *frame, uint8_t port)
{
  mrfiPacket_t dFrame;
  uint8_t      tid = GET_FROM_FRAME(MRFI_P_PAYLOAD(frame), F_TRACTID_OS);

  /* set the type of device sending the frame in the header */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_TX_DEVICE, sMyTxType);

  /* set the listen type of device sending the frame in the header. */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_RX_TYPE, sMyRxType);

  /* destination address from received frame */
  memcpy(MRFI_P_DST_ADDR(&dFrame), MRFI_P_SRC_ADDR(frame), NET_ADDR_SIZE);

  /* source address */
  memcpy(MRFI_P_SRC_ADDR(&dFrame), sMyAddr, NET_ADDR_SIZE);

  /* port is the source the Tx port from the connection object */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_PORT_OS, port);

  /* frame length... */
  MRFI_SET_PAYLOAD_LEN(&dFrame,F_APP_PAYLOAD_OS);

  /* transaction ID taken from source frame */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_TRACTID_OS, tid);

  /* hop count... */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_HOP_COUNT, MAX_HOPS);

  /* set ACK field */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ACK_RPLY, F_ACK_RPLY_TYPE);
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ACK_REQ, 0);

   /* This is not a forwarded frame */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_FWD_FRAME, 0);

  /* Encryption state */
#if !defined(SMPL_SECURE)
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ENCRYPT_OS, 0);
#else
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ENCRYPT_OS, F_ENCRYPT_OS_MSK);
  nwk_setSecureFrame(&dFrame, 0, 0);
#endif

  MRFI_Transmit(&dFrame, MRFI_TX_TYPE_FORCED);

  return;
}
#endif /* APP_AUTO_ACK */

#if !defined(END_DEVICE)
/******************************************************************************
 * @fn          nwk_replayFrame
 *
 * @brief       Deal with hop count on a Range Extender or Access Point replay.
 *              Queue entry usage always left as available when done.
 *
 * input parameters
 * @param   pFrameInfo   - pointer to frame information structure
 *
 * output parameters
 *
 * @return      void
 */
void nwk_replayFrame(frameInfo_t *pFrameInfo)
{
  uint8_t  hops = GET_FROM_FRAME(MRFI_P_PAYLOAD(&pFrameInfo->mrfiPkt), F_HOP_COUNT);

  /* if hops are zero, drop frame. othewise send it. */
  if (hops--)
  {
    PUT_INTO_FRAME(MRFI_P_PAYLOAD(&pFrameInfo->mrfiPkt),F_HOP_COUNT,hops);
    /* Don't care if the Tx fails because of TO. Either someone else
     * will retransmit or the application itself will recover.
     */
#if defined(SMPL_SECURE)
    /* If the frame was targeted to a NWK port it was decrypted on spec in
     * the 'dispatchFrame()' method. It must be re-encypted in this case.
     */
    if (GET_FROM_FRAME(MRFI_P_PAYLOAD(&pFrameInfo->mrfiPkt), F_PORT_OS) <= SMPL_PORT_NWK_BCAST)
    {
      nwk_setSecureFrame(&pFrameInfo->mrfiPkt, MRFI_GET_PAYLOAD_LEN(&pFrameInfo->mrfiPkt)-F_APP_PAYLOAD_OS, 0);
    }
#endif
    MRFI_DelayMs(1);
    nwk_sendFrame(pFrameInfo, MRFI_TX_TYPE_CCA);
  }
  else
  {
    pFrameInfo->fi_usage = FI_AVAILABLE;
  }
  return;
}

#if defined(ACCESS_POINT)
/******************************************************************************
 * @fn          nwk_getSandFFrame
 *
 * @brief       Get any frame waiting for the client on the port supplied in
 *              the frame payload.
 *              TODO: support returning NWK application frames always. the
 *              port requested in the call should be an user application port.
 *              NWK app ports will never be in the called frame.
 *              TODO: deal with broadcast NWK frames from AP.
 *
 * input parameters
 * @param   frame   - pointer to frame in question
 *
 * output parameters
 *
 * @return      pointer to frame if there is one, otherwise 0.
 */
frameInfo_t *nwk_getSandFFrame(mrfiPacket_t *frame, uint8_t osPort)
{
  uint8_t        i, port = *(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+osPort);
  frameInfo_t *fiPtr;
  rcvContext_t rcv;

  rcv.type  = RCV_RAW_POLL_FRAME;
  rcv.t.pkt = frame;
  /* check the input queue for messages sent by others. */
  if (fiPtr=nwk_QfindOldest(INQ, &rcv, USAGE_FWD))
  {
    return fiPtr;
  }

  /* Check the output queue to see if we ourselves need to send anything.
   * TODO: use the cast-out scheme for output queue so this routine finds
   * the oldest in either queue.
   */
  fiPtr = nwk_getQ(OUTQ);
  for (i=0; i<SIZE_OUTFRAME_Q; ++i, fiPtr++)
  {
    if (FI_INUSE_UNTIL_FWD == fiPtr->fi_usage)
    {
      if (!memcmp(MRFI_P_DST_ADDR(&fiPtr->mrfiPkt), MRFI_P_SRC_ADDR(frame), NET_ADDR_SIZE))
      {
        if (GET_FROM_FRAME(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), F_PORT_OS) == port)
        {
          return fiPtr;
        }
      }
    }
  }
  return 0;
}

/******************************************************************************
 * @fn          nwk_SendEmptyPollRspFrame
 *
 * @brief       There are no frames waiting for the requester on the specified
 *              port. Send a frame back to that port with no payload.
 *
 * input parameters
 * @param   frame   - pointer to frame in question
 *
 * output parameters
 *
 * @return      void
 */
void nwk_SendEmptyPollRspFrame(mrfiPacket_t *frame)
{
  mrfiPacket_t dFrame;
  uint8_t      port = *(MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+M_POLL_PORT_OS);

  /* set the type of device sending the frame in the header. we know it's an AP */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_TX_DEVICE, F_TX_DEVICE_AP);
  /* set the listen type of device sending the frame in the header. we know it's
   * an AP is is probably always on...but use the static variable anyway.
   */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_RX_TYPE, sMyRxType);
  /* destination address from received frame (polling device) */
  memcpy(MRFI_P_DST_ADDR(&dFrame), MRFI_P_SRC_ADDR(frame), NET_ADDR_SIZE);
  /* source address */
  memcpy(MRFI_P_SRC_ADDR(&dFrame), MRFI_P_PAYLOAD(frame)+F_APP_PAYLOAD_OS+M_POLL_ADDR_OS, NET_ADDR_SIZE);
  /* port is the port requested */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_PORT_OS, port);
  /* frame length... */
  MRFI_SET_PAYLOAD_LEN(&dFrame,F_APP_PAYLOAD_OS);
  /* transaction ID... */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_TRACTID_OS, sTRACTID);
  sTRACTID++;
  /* hop count... */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_HOP_COUNT, MAX_HOPS_FROM_AP);

  /* Ack fields */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ACK_RPLY, 0);
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ACK_REQ, 0);

  /* This is logically a forwarded frame */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_FWD_FRAME, F_FRAME_FWD_TYPE);

  /* Encryption state */
#if !defined(SMPL_SECURE)
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ENCRYPT_OS, 0);
#else
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(&dFrame), F_ENCRYPT_OS, F_ENCRYPT_OS_MSK);
  nwk_setSecureFrame(&dFrame, 0, 0);
#endif

  MRFI_Transmit(&dFrame, MRFI_TX_TYPE_FORCED);

  return;
}

/******************************************************************************
 * @fn          isDupSandFFrame
 *
 * @brief       Have we already stored this frame on behalf of a client?
 *
 * input parameters
 * @param   frame   - pointer to frame in question
 *
 * output parameters
 *
 * @return      Returns 1 if the frame is a duplicate, otherwise 0.
 */
uint8_t  isDupSandFFrame(mrfiPacket_t *frame)
{
  uint8_t      i, plLen = MRFI_GET_PAYLOAD_LEN(frame);
  frameInfo_t *fiPtr;

  /* check the input queue for duplicate S&F frame. */
  fiPtr = nwk_getQ(INQ);
  for (i=0; i<SIZE_INFRAME_Q; ++i, fiPtr++)
  {
    if (FI_INUSE_UNTIL_FWD == fiPtr->fi_usage)
    {
      /* compare everything except the DEVICE INFO byte. */
      if (MRFI_GET_PAYLOAD_LEN(&fiPtr->mrfiPkt) == plLen                                   &&
          !memcmp(MRFI_P_DST_ADDR(&fiPtr->mrfiPkt), MRFI_P_DST_ADDR(frame), NET_ADDR_SIZE) &&
          !memcmp(MRFI_P_SRC_ADDR(&fiPtr->mrfiPkt), MRFI_P_SRC_ADDR(frame), NET_ADDR_SIZE) &&
          !memcmp(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt), MRFI_P_PAYLOAD(frame), 1)               &&
          !memcmp(MRFI_P_PAYLOAD(&fiPtr->mrfiPkt)+F_TRACTID_OS, MRFI_P_PAYLOAD(frame)+F_TRACTID_OS, plLen-F_TRACTID_OS)
          )
      {
        return 1;
      }
    }
  }
  return 0;
}
#endif  /* ACCESS_POINT */

#endif  /* !END_DEVICE */
