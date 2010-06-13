/**************************************************************************************************
  Filename:       nwk_globals.c
  Revised:        $Date: 2009-10-27 20:48:11 -0700 (Tue, 27 Oct 2009) $
  Revision:       $Revision: 20995 $

  Description:    This file manages global NWK data.

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
#include "nwk_globals.h"

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
static const addr_t   sMyROMAddress = THIS_DEVICE_ADDRESS;
static addr_t         sAPAddress;
static addr_t         sMyRAMAddress;
static uint8_t        sRAMAddressIsSet = 0;

/* Version number set as a 4 byte quantity. Each byte is a revision number
 * in the form w.x.y.z. The subfields are each limited to values 0x0-0xFF.
 */
static const smplVersionInfo_t sVersionInfo = {
                                                0x02,  /* protocol version */
                                                0x01,  /* major revision number */
                                                0x01,  /* minor revision number */
                                                0x01,  /* maintenance release number */
                                                0x00   /* special release */
                                               };

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
 * @fn          nwk_globalsInit
 *
 * @brief       Initialization of global symbols
 *
 * input parameters
 *
 * output parameters
 *
 * @return   void
 */
void nwk_globalsInit(void)
{

  memset(&sAPAddress, 0x00, sizeof(addr_t));

  /* populate RAM address from ROM default if it hasn't laready been set
   * using the IOCTL interface.
   */
  if (!sRAMAddressIsSet)
  {
    memcpy(&sMyRAMAddress, &sMyROMAddress, sizeof(addr_t));
    sRAMAddressIsSet = 1;  /* RAM address is now valid */
  }

  return;
}

/******************************************************************************
 * @fn          nwk_getMyAddress
 *
 * @brief       Return a pointer to my address. It comes either from ROM as
 *              set in the configuration file or it was set using the IOCTL
 *              interface -- probably from random bytes.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   pointer to a constant address type object.
 */
addr_t const *nwk_getMyAddress(void)
{
  /* This call supports returning a valid pointer before either the
   * initialization or external setting of the address. But caller needs
   * to be careful -- if this routine is called immediately it will return
   * the ROM address. If the application then sets the address using the
   * IOCTL before doing the SMPL_Init() the original pointer is no longer
   * valid as it points to the wrong address.
   */
  return sRAMAddressIsSet ? &sMyRAMAddress : &sMyROMAddress;
}

/******************************************************************************
 * @fn          nwk_getFWVersion
 *
 * @brief       Return a pointer to the current firmware version string.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   pointer to a constant uint16_t object.
 */
uint8_t const *nwk_getFWVersion()
{
  return sVersionInfo.fwVerString;
}

/******************************************************************************
 * @fn          nwk_getProtocolVersion
 *
 * @brief       Return the current protocol version.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   Protocol version.
 */
uint8_t nwk_getProtocolVersion(void)
{
  return sVersionInfo.protocolVersion;
}

/******************************************************************************
 * @fn          nwk_setMyAddress
 *
 * @brief       Set my address object if it hasn't already been set. This call
 *              is referenced by the IOCTL support used to change the device
 *              address. It is effective only if the address has not already
 *              been set.
 *
 * input parameters
 *
 * @param   addr  - pointer to the address object to be used to set my address.
 *
 * output parameters
 *
 * @return   Returns non-zero if request is respected, otherwise returns 0.
 */
uint8_t nwk_setMyAddress(addr_t *addr)
{
  uint8_t rc = 0;

  if (!sRAMAddressIsSet)
  {
    memcpy(&sMyRAMAddress, addr, sizeof(addr_t));
    sRAMAddressIsSet = 1;  /* RAM address is now valid */
    rc = 1;
  }

  return rc;
}

/******************************************************************************
 * @fn          nwk_setAPAddress
 *
 * @brief       Set the AP's address. Called after the AP address is gleaned
 *              from the Join reply.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   void
 */
void nwk_setAPAddress(addr_t *addr)
{

  memcpy((void *)&sAPAddress, (void *)addr, NET_ADDR_SIZE);

  return;
}

/******************************************************************************
 * @fn          nwk_getAPAddress
 *
 * @brief       Get the AP's address.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   Pointer to a constant address object or null if the address has not
 *           yet been set.
 */
addr_t const *nwk_getAPAddress(void)
{
  addr_t addr;

  memset(&addr, 0x0, sizeof(addr));

  return !memcmp(&sAPAddress, &addr, NET_ADDR_SIZE) ? 0 : &sAPAddress;
}

/******************************************************************************
 * @fn          nwk_getBCastAddress
 *
 * @brief       Get the network broadcast address.
 *
 * input parameters
 *
 * output parameters
 *
 * @return   Pointer to a constant address object.
 */
addr_t const *nwk_getBCastAddress(void)
{
  return (addr_t const *)mrfiBroadcastAddr;
}
