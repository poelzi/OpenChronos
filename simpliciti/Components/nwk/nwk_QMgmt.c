/**************************************************************************************************
  Filename:       nwk_QMgmt.c
  Revised:        $Date: 2009-03-10 17:01:56 -0700 (Tue, 10 Mar 2009) $
  Revision:       $Revision: 19372 $
  Author:         $Author: lfriedman $

  Description:    This file supports the SimpliciTI input and output frame queues

  Copyright 2007-2008 Texas Instruments Incorporated. All rights reserved.

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
#include <intrinsics.h>
#include <string.h>
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk.h"
#include "nwk_frame.h"
#include "nwk_QMgmt.h"
#include "nwk_mgmt.h"     /* need offsets for poll frames */

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
static frameInfo_t   sInFrameQ[SIZE_INFRAME_Q];
#else
static frameInfo_t  *sInFrameQ = NULL;
#endif  /* SIZE_INFRAME_Q > 0 */

static frameInfo_t   sOutFrameQ[SIZE_OUTFRAME_Q];

/******************************************************************************
 * LOCAL FUNCTIONS
 */

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * GLOBAL FUNCTIONS
 */

/******************************************************************************
* @fn          nwk_QInit
* 
* @brief     Initialize the input and output frame queues to hold no packets. 
* 
* input parameters
* 
* output parameters
* 
* @return   void
*/
void nwk_QInit(void)
{
#if SIZE_INFRAME_Q > 0
  memset(sInFrameQ, 0, sizeof(sInFrameQ));
#endif  // SIZE_INFRAME_Q > 0
  memset(sOutFrameQ, 0, sizeof(sOutFrameQ));
}
 
/******************************************************************************
 * @fn          nwk_QfindSlot
 *
 * @brief       Finds a slot to use to retrieve the frame from the radio. It
 *              uses a LRU cast-out scheme. It is possible that this routine
 *              finds no slot. This can happen if the queue is of size 1 or 2
 *              and the Rx interrupt occurs during a retrieval call from an
 *              application. There are meta-states for frames as the application
 *              looks for the oldest frame on the port being requested.
 *
 *              This routine is running in interrupt context.
 *
 * input parameters
 * @param   which   - INQ or OUTQ to search
 *
 * output parameters
 *
 * @return      Pointer to oldest available frame in the queue
 */
frameInfo_t *nwk_QfindSlot(uint8_t which)
{
  frameInfo_t *pFI, *oldest= 0, *newFI = 0;
  uint8_t        i, num, newOrder = 0, orderTest;

  if (INQ == which)
  {
    pFI  = sInFrameQ;
    num  = SIZE_INFRAME_Q;
  }
  else
  {
    pFI  = sOutFrameQ;
    num  = SIZE_OUTFRAME_Q;
  }

  orderTest = num + 1;

  for (i=0; i<num; ++i, ++pFI)
  {
    /* if frame is available it's a candidate. */
    if (pFI->fi_usage != FI_AVAILABLE)
    {
      if (INQ == which)  /* TODO: do cast-out for Tx as well */
      {

        /* need to know the number of occupied slots so we know the age value
         * for the unoccupied slot (if there is one).
         */
        newOrder++;

        /* make sure nwk_retrieveFrame() is not processing this frame */
        if (FI_INUSE_TRANSITION == pFI->fi_usage)
        {
          continue;
        }
        /* is this frame older than any we've seen? */
        if (orderTest > pFI->orderStamp)
        {
          /* yes. */
          oldest    = pFI;
          orderTest = pFI->orderStamp;
        }
      }
    }
    else
    {
      if (OUTQ == which)  /* TODO: do cast-out for Tx as well */
      {
        return pFI;
      }
      newFI = pFI;
    }
  }

  /* did we find anything? */
  if (!newFI)
  {
    /* queue was full. cast-out happens here...unless... */
    if (!oldest)
    {
      /* This can happen if the queue is only of size 1 or 2 and all
       * the frames are in transition when the Rx interrupt occurs.
       */
      return (frameInfo_t *)0;
    }
    newFI = oldest;
    nwk_QadjustOrder(which, newFI->orderStamp);
    newFI->orderStamp = i;
  }
  else
  {
    /* mark the available slot. */
    newFI->orderStamp = ++newOrder;
  }

  return newFI;
}

/******************************************************************************
 * @fn          nwk_QadjustOrder
 *
 * @brief       Adjusts the age of everyone in the queue newer than the frame
 *              being removed.
 *
 * input parameters
 * @param   which   - INQ or OUTQ to adjust
 * @param   stamp   - value of frame being removed
 *
 * output parameters
 *
 * @return      void
 */
void nwk_QadjustOrder(uint8_t which, uint8_t stamp)
{
  frameInfo_t *pFI;
  uint8_t      i, num;
  bspIState_t  intState;

  if (INQ == which)
  {
    pFI  = sInFrameQ;
    num  = SIZE_INFRAME_Q;
  }
  else
  {
/*    pFI  = sOutFrameQ; */
/*    num  = SIZE_OUTFRAME_Q; */
    return;
  }

  BSP_ENTER_CRITICAL_SECTION(intState);

  for (i=0; i<num; ++i, ++pFI)
  {
    if ((pFI->fi_usage != FI_AVAILABLE) && (pFI->orderStamp > stamp))
    {
      pFI->orderStamp--;
    }
  }

  BSP_EXIT_CRITICAL_SECTION(intState);

  return;
}

/******************************************************************************
 * @fn          nwk_QfindOldest
 *
 * @brief       Look through frame queue and find the oldest available frame
 *              in the context in question. Supports connection-based (user),
 *              non-connection based (NWK applications), and the special case
 *              of store-and-forward.
 *
 * input parameters
 * @param   which      - INQ or OUTQ to adjust
 * @param   rcvContext - context information for finding the oldest
 * @param   usage      - normal usage or store-and-forward usage
 *
 * output parameters
 *
 * @return      Pointer to frame that is the oldsest on the requested port, or
 *              0 if there are none.
 */
frameInfo_t *nwk_QfindOldest(uint8_t which, rcvContext_t *rcv, uint8_t fi_usage)
{
  uint8_t      i, oldest, num, port;
  uint8_t      uType, addr12Compare;
  bspIState_t  intState;
  frameInfo_t *fPtr = 0, *wPtr;
  connInfo_t  *pCInfo = 0;
  uint8_t     *pAddr1, *pAddr2, *pAddr3 = 0;

  if (INQ == which)
  {
    wPtr   = sInFrameQ;
    num    = SIZE_INFRAME_Q;
    oldest = SIZE_INFRAME_Q+1;
  }
  else
  {
/*    pFI  = sOutFrameQ; */
/*    num  = SIZE_OUTFRAME_Q; */
    return 0;
  }

  if (RCV_APP_LID == rcv->type)
  {
    pCInfo = nwk_getConnInfo(rcv->t.lid);
    if (!pCInfo)
    {
      return (frameInfo_t *)0;
    }
    port   = pCInfo->portRx;
    pAddr2 = pCInfo->peerAddr;
  }
  else if (RCV_NWK_PORT == rcv->type)
  {
    port = rcv->t.port;
  }
#ifdef ACCESS_POINT
  else if (RCV_RAW_POLL_FRAME == rcv->type)
  {
    port   = *(MRFI_P_PAYLOAD(rcv->t.pkt)+F_APP_PAYLOAD_OS+M_POLL_PORT_OS);
    pAddr2 = MRFI_P_SRC_ADDR(rcv->t.pkt);
    pAddr3 = MRFI_P_PAYLOAD(rcv->t.pkt)+F_APP_PAYLOAD_OS+M_POLL_ADDR_OS;
  }
#endif
  else
  {
    return (frameInfo_t *)0;
  }

  uType = (USAGE_NORMAL == fi_usage) ? FI_INUSE_UNTIL_DEL : FI_INUSE_UNTIL_FWD;

  for (i=0; i<num; ++i, ++wPtr)
  {

    BSP_ENTER_CRITICAL_SECTION(intState);   /* protect the frame states */

    /* only check entries in use and waiting for this port */
    if (uType == wPtr->fi_usage)
    {
      wPtr->fi_usage = FI_INUSE_TRANSITION;

      BSP_EXIT_CRITICAL_SECTION(intState);  /* release hold */
      /* message sent to this device? */
      if (GET_FROM_FRAME(MRFI_P_PAYLOAD(&wPtr->mrfiPkt), F_PORT_OS) == port)
      {
        /* Port matches. If the port of interest is a NWK applicaiton we're a
         * match...the NWK applications are not connection-based. If it is a
         * NWK application we need to check the source address for disambiguation.
         * Also need to check source address if it's a raw frame lookup (S&F frame)
         */
        if (RCV_APP_LID == rcv->type)
        {
          if (SMPL_PORT_USER_BCAST == port)
          {
            /* guarantee a match... */
            pAddr1 = pCInfo->peerAddr;
          }
          else
          {
            pAddr1 = MRFI_P_SRC_ADDR(&wPtr->mrfiPkt);
          }
        }
#ifdef ACCESS_POINT
        else if (RCV_RAW_POLL_FRAME == rcv->type)
        {
          pAddr1 = MRFI_P_DST_ADDR(&wPtr->mrfiPkt);
        }
#endif

        addr12Compare = memcmp(pAddr1, pAddr2, NET_ADDR_SIZE);
        if (  (RCV_NWK_PORT == rcv->type) ||
              (!pAddr3 && !addr12Compare) ||
              (pAddr3 && !memcmp(pAddr3, MRFI_P_SRC_ADDR(&wPtr->mrfiPkt), NET_ADDR_SIZE))
           )
        {
          if (wPtr->orderStamp < oldest)
          {
            if (fPtr)
            {
              /* restore previous oldest one */
              fPtr->fi_usage = uType;
            }
            oldest = wPtr->orderStamp;
            fPtr   = wPtr;
            continue;
          }
          else
          {
            /* not oldest. restore state */
            wPtr->fi_usage = uType;
          }
        }
        else
        {
          /* not a match. restore state */
          wPtr->fi_usage = uType;
        }
      }
      else
      {
        /* wrong port. restore state */
        wPtr->fi_usage = uType;
      }
    }
    else
    {
      BSP_EXIT_CRITICAL_SECTION(intState);
    }
  }

  return fPtr;
}

/******************************************************************************
 * @fn          nwk_getQ
 *
 * @brief       Get location of teh specified frame queue.
 *
 * input parameters
 * @param   which   - INQ or OUTQ to get
 *
 * output parameters
 *
 * @return      Pointer to frame queue
 */
frameInfo_t *nwk_getQ(uint8_t which)
{
  return (INQ == which) ? sInFrameQ : sOutFrameQ;
}

