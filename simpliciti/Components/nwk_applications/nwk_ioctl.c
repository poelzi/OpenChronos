
/**************************************************************************************************
  Filename:       nwk_ioctl.c
  Revised:        $Date: 2009-01-13 11:31:14 -0800 (Tue, 13 Jan 2009) $
  Revision:       $Revision: 18744 $
  Author:         $Author: lfriedman $

  Description:    This file supports the SimpliciTI IOCTL implmentation. This interface
                  gives applications access to the "driver" network level functions
                  when necessary.

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


/******************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_frame.h"
#include "nwk.h"
#include "nwk_ioctl.h"
#include "nwk_globals.h"
#include "nwk_security.h"
#ifdef ACCESS_POINT
#include "nwk_join.h"
#endif

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
 * @fn          nwk_rawSend
 *
 * @brief       Builds an outut frame based on information provided by the
 *              caller. This function allows a raw transmission to the target
 *              if the network address is known. this function is used a lot
 *              to support NWK applications.
 *
 * input parameters
 * @param   info    - pointer to strcuture containing info on how to build
 *                    the outgoing frame.
 * output parameters
 *
 * @return         SMPL_SUCCESS
 *                 SMPL_NOMEM       - no room in output frame queue
 *                 SMPL_TX_CCA_FAIL - CCA failure
 */
smplStatus_t nwk_rawSend(ioctlRawSend_t *info)
{
  frameInfo_t *pOutFrame;
  uint8_t      hops;

  /* If we know frame is going to or from the AP then we can reduce the hop
   * count.
   */
  switch (info->port)
  {
    case SMPL_PORT_JOIN:
    case SMPL_PORT_FREQ:
    case SMPL_PORT_MGMT:
      hops = MAX_HOPS_FROM_AP;
      break;

    default:
      hops = MAX_HOPS;
      break;
  }

  if (pOutFrame = nwk_buildFrame(info->port, info->msg, info->len, hops))
  {
    memcpy(MRFI_P_DST_ADDR(&pOutFrame->mrfiPkt), info->addr, NET_ADDR_SIZE);
#ifdef SMPL_SECURE
    nwk_setSecureFrame(&pOutFrame->mrfiPkt, info->len, 0);
#endif  /* SMPL_SECURE */
    return nwk_sendFrame(pOutFrame, MRFI_TX_TYPE_CCA);
  }
  return SMPL_NOMEM;
}

/******************************************************************************
 * @fn          nwk_rawReceive
 *
 * @brief       Retriievs specified from from the input frame queue. Additional
 *              information such as source address and hop count may also be
 *              retrieved
 *
 * input parameters
 * @param   info    - pointer to structure containing info on what to retrieve
 *
 * output parameters - actually populated by nwk_retrieveFrame()
 *      info->msg      - application payload copied here
 *      info->len      - length of received application payload
 *      info->addr     - if non-NULL points to memory to be populated with
 *                       source address of retrieved frame.
 *      info->hopCount - if non-NULL points to memory to be populated with
 *                       hop count of retrieved frame.
 *
 * @return   Status of operation.
 */
smplStatus_t nwk_rawReceive(ioctlRawReceive_t *info)
{
  rcvContext_t rcv;

  rcv.type   = RCV_NWK_PORT;
  rcv.t.port = info->port;

  return nwk_retrieveFrame(&rcv, info->msg, &info->len, info->addr, &info->hopCount);
}

/******************************************************************************
 * @fn          nwk_radioControl
 *
 * @brief       Handle radio control functions.
 *
 * input parameters
 * @param   action   - radio operation to perform. currently suppoerted:
 *                         sleep/unsleep
 * output parameters
 *
 * @return   Status of operation.
 */
smplStatus_t nwk_radioControl(ioctlAction_t action, void *val)
{
  smplStatus_t rc = SMPL_SUCCESS;

  if (IOCTL_ACT_RADIO_SLEEP == action)
  {
    /* go to sleep mode. */
    MRFI_RxIdle();
    MRFI_Sleep();
  }
  else if (IOCTL_ACT_RADIO_AWAKE == action)
  {
    MRFI_WakeUp();

#if !defined( END_DEVICE )
    MRFI_RxOn();
#endif

  }
  else if (IOCTL_ACT_RADIO_SIGINFO == action)
  {
    ioctlRadioSiginfo_t *pSigInfo = (ioctlRadioSiginfo_t *)val;
    connInfo_t          *pCInfo   = nwk_getConnInfo(pSigInfo->lid);

    if (!pCInfo)
    {
      return SMPL_BAD_PARAM;
    }
    memcpy(&pSigInfo->sigInfo, &pCInfo->sigInfo, sizeof(pCInfo->sigInfo));
  }
  else if (IOCTL_ACT_RADIO_RSSI == action)
  {
    *((rssi_t *)val) = MRFI_Rssi();
  }
  else if (IOCTL_ACT_RADIO_RXON == action)
  {
    MRFI_RxOn();
  }
  else if (IOCTL_ACT_RADIO_RXIDLE == action)
  {
    MRFI_RxIdle();
  }
#ifdef EXTENDED_API
  else if (IOCTL_ACT_RADIO_SETPWR == action)
  {
    uint8_t idx;

    switch (*(ioctlLevel_t *)val)
    {
      case IOCTL_LEVEL_2:
        idx = 2;
        break;

      case IOCTL_LEVEL_1:
        idx = 1;
        break;

      case IOCTL_LEVEL_0:
        idx = 0;
        break;

      default:
        return SMPL_BAD_PARAM;
    }
    MRFI_SetRFPwr(idx);
    return SMPL_SUCCESS;
  }
#endif  /* EXTENDED_API */
  else
  {
    rc = SMPL_BAD_PARAM;
  }
  return rc;
}

/******************************************************************************
 * @fn          nwk_joinContext
 *
 * @brief       For Access Points we need a way to support changing the Join
 *              context. This will allow arbitration bewteen potentially nearby
 *              Access Points when a new device is joining.
 *
 * input parameters
 * @param   action  - Join context is either on or off.
 *
 * output parameters
 *
 * @return   Status of operation. Currently always succeeds.
 */
#ifdef ACCESS_POINT
smplStatus_t nwk_joinContext(ioctlAction_t action)
{
  nwk_setJoinContext((IOCTL_ACT_ON == action) ? JOIN_CONTEXT_ON : JOIN_CONTEXT_OFF);

  return SMPL_SUCCESS;
}
#endif

/******************************************************************************
 * @fn          nwk_deviceAddress
 *
 * @brief       Set or Get this device address. The Set must be done before
 *              SMPL_Init() for it to take effect. The Get is always legal but
 *              the value could be invalid if it is called before a valid set
 *              call is made.
 *
 * input parameters
 * @param   action  - Gte or Set
 * @param   addr    - pointer to address object containing value on Set
 *
 * output parameters
 * @param   addr    - pointer to address object to receive value on Get.
 *
 * @return   SMPL_SUCCESS
 *           SMPL_BAD_PARAM  Action request illegal or a Set request
 *                           was not respected.
 */
smplStatus_t nwk_deviceAddress(ioctlAction_t action, addr_t *addr)
{
  smplStatus_t rc = SMPL_BAD_PARAM;

  if (IOCTL_ACT_GET == action)
  {
    memcpy(addr, nwk_getMyAddress(), sizeof(addr_t));
    rc = SMPL_SUCCESS;
  }
  else if (IOCTL_ACT_SET == action)
  {
    if (nwk_setMyAddress(addr))
    {
      rc = SMPL_SUCCESS;
    }
  }

  return rc;
}

/******************************************************************************
 * @fn          nwk_connectionControl
 *
 * @brief       Access to connection table. Currently supports only deleting
 *              a connection from the table.
 *
 * input parameters
 * @param   action  - Connection control action (only delete is curently valid).
 * @param   val     - pointer to Link ID of connection on which to operate.
 *
 * output parameters
 *
 * @return   SMPL_SUCCESS
 *           SMPL_BAD_PARAM  Action is not delete
 *                           Link ID is the UUD Link ID
 *                           No connection table info for Link ID
 */
smplStatus_t nwk_connectionControl(ioctlAction_t action, void *val)
{
  connInfo_t *pCInfo;
  linkID_t    lid = *((linkID_t *)val);

  if (IOCTL_ACT_DELETE != action)
  {
    return SMPL_BAD_PARAM;
  }

  if ((SMPL_LINKID_USER_UUD == lid) ||
      (!(pCInfo=nwk_getConnInfo(lid))))
  {
    return SMPL_BAD_PARAM;
  }

  nwk_freeConnection(pCInfo);

  return SMPL_SUCCESS;
}
