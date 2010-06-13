/**************************************************************************************************
  Revised:        $Date: 2009-11-23 07:50:43 -0800 (Mon, 23 Nov 2009) $
  Revision:       $Revision: 21225 $

  Copyright 2008-2009 Texas Instruments Incorporated.  All rights reserved.

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

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS?
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

/* ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 *   MRFI (Minimal RF Interface)
 *   Radios: CC430
 *   Radio Interface (RIF) code.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include "mrfi_radio_interface.h"


/* ------------------------------------------------------------------------------------------------
 *                                            Defines
 * ------------------------------------------------------------------------------------------------
 */

/* ------------------------------------------------------------------------------------------------
 *                                            Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_RADIO_STATUS_READ_CLEAR()  RF1AIFCTL1 &= ~(RFSTATIFG);

#define MRFI_RADIO_STATUS_READ_WAIT()  while( !(RF1AIFCTL1 & RFSTATIFG) );
#define MRFI_RADIO_INST_WRITE_WAIT()   while( !(RF1AIFCTL1 & RFINSTRIFG));
#define MRFI_RADIO_DATA_WRITE_WAIT()   while( !(RF1AIFCTL1 & RFDINIFG)  );
#define MRFI_RADIO_DATA_READ_WAIT()    while( !(RF1AIFCTL1 & RFDOUTIFG) );

#define MRFI_RIF_DEBUG
#ifdef MRFI_RIF_DEBUG
#define MRFI_RIF_ASSERT(x)      BSP_ASSERT(x)
#else
#define MRFI_RIF_ASSERT(x)
#endif


/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */


/**************************************************************************************************
 * @fn          mrfiRadioInterfaceInit
 *
 * @brief       Initialize the Radio Interface
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiRadioInterfaceInit(void)
{
  /* Enable interrupt on interface error.
   * The code behaves differently between different runs on the debugger, and seemingly fails
   * due to error flags on the bench. The code does not fail, however, on functional tests.
   * This points to problems with the debugger that need to be sorted through carefully.
   * For the time being, remove the following line, since it will likely cause operational
   * failures.
   */
  // RF1AIFCTL1 |= RFERRIE;
}


/**************************************************************************************************
 * @fn          mrfiRadioInterfaceCmdStrobe
 *
 * @brief       Send command strobe to the radio.  Returns status byte read during transfer
 *              of strobe command.
 *
 * @param       addr - address of register to strobe
 *
 * @return      status byte of radio
 **************************************************************************************************
 */
uint8_t mrfiRadioInterfaceCmdStrobe(uint8_t addr)
{
  uint8_t statusByte, gdoState;
  mrfiRIFIState_t s;

  /* Check for invalid address.
   * 0xBD is for SNOP with MSP set to read the bytes available in RX FIFO.
   */
  MRFI_RIF_ASSERT( (addr == 0xBD) || (addr >= RF_SRES) && (addr <= RF_SNOP));

  /* Lock out access to Radio IF */
  MRFI_RIF_ENTER_CRITICAL_SECTION(s);

  /* Clear the Status read flag */
  MRFI_RADIO_STATUS_READ_CLEAR();

  /* Wait for radio to be ready for next instruction */
  MRFI_RADIO_INST_WRITE_WAIT();

  if ((addr > RF_SRES) && (addr < RF_SNOP))
  {
    /* buffer IOCFG2 state */
    gdoState = MRFI_RADIO_REG_READ(IOCFG2);

    /* c-ready to GDO2 */
    MRFI_RADIO_REG_WRITE(IOCFG2, 0x29);

    RF1AINSTRB = addr;

    /* chip at sleep mode */
    if ((RF1AIN & 0x04) == 0x04)
    {
      if ( (addr == RF_SXOFF) || (addr == RF_SPWD) || (addr == RF_SWOR) )
      {
        /* Do nothing */
      }
      else
      {
        /* c-ready */
        while ((RF1AIN & 0x04) == 0x04);

        /* Delay should be 760us */
        Mrfi_DelayUsec(760);
      }
    }

    /* restore IOCFG2 setting */
    MRFI_RADIO_REG_WRITE(IOCFG2, gdoState);
  }
  else
  {
    /* chip active mode */
    RF1AINSTRB = addr;
  }

  /* Read status byte */
  statusByte = RF1ASTAT0B;

  /* Allow access to Radio IF */
  MRFI_RIF_EXIT_CRITICAL_SECTION(s);

  /* return the status byte */
  return statusByte;
}


/**************************************************************************************************
 * @fn          mrfiRadioInterfaceReadReg
 *
 * @brief       Read value from radio register.
 *
 * @param       addr - address of register
 *
 * @return      register value
 **************************************************************************************************
 */
uint8_t mrfiRadioInterfaceReadReg(uint8_t addr)
{
  mrfiRIFIState_t s;
  uint8_t regValue;

  /* Check for valid range. 0x3E is for PATABLE access */
  MRFI_RIF_ASSERT( (addr <= 0x3B) || (addr == 0x3E) );

  /* Lock out access to Radio IF */
  MRFI_RIF_ENTER_CRITICAL_SECTION(s);

  /* Wait for radio to be ready for next instruction */
  MRFI_RADIO_INST_WRITE_WAIT();

  if( (addr <= 0x2E) || (addr == 0x3E))
  {
    /* Write cmd: read the Configuration register */
    RF1AINSTR1B = (0x80 | addr);
  }
  else
  {
    /* Write cmd: read the Status register */
    RF1AINSTR1B = (0xC0 | addr);
  }

  /* Read out the register value */
  regValue   = RF1ADOUT1B; //auto read

  /* Allow access to Radio IF */
  MRFI_RIF_EXIT_CRITICAL_SECTION(s);

  return( regValue);
}


/**************************************************************************************************
 * @fn          mrfiRadioInterfaceWriteReg
 *
 * @brief       Write value to radio register.
 *
 * @param       addr  - address of register
 * @param       value - register value to write
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiRadioInterfaceWriteReg(uint8_t addr, uint8_t value)
{
  mrfiRIFIState_t s;

  /* Check for valid range. 0x3E is for PATABLE access */
  MRFI_RIF_ASSERT((addr <= 0x2E) || (addr == 0x3E));

  /* Lock out access to Radio IF */
  MRFI_RIF_ENTER_CRITICAL_SECTION(s);

  /* Wait for radio to be ready for next instruction */
  MRFI_RADIO_INST_WRITE_WAIT();

  /* Write cmd: 'write to register' */
  RF1AINSTRB = (0x00 | addr);

  /* Wait for radio to be ready to accept the data */
  MRFI_RADIO_DATA_WRITE_WAIT();

  /* Write the register value */
  RF1ADINB   = value; /* value to be written to the radio register */

  /* Allow access to Radio IF */
  MRFI_RIF_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          mrfiRadioInterfaceWriteTxFifo
 *
 * @brief       Write data to radio transmit FIFO.
 *
 * @param       pData - pointer for storing write data
 * @param       len   - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
void mrfiRadioInterfaceWriteTxFifo(uint8_t * pData, uint8_t len)
{
  mrfiRIFIState_t s;

  MRFI_RIF_ASSERT(len != 0); /* zero length is not allowed */

  /* Lock out access to Radio IF */
  MRFI_RIF_ENTER_CRITICAL_SECTION(s);

  /* Wait for radio to be ready for next instruction */
  MRFI_RADIO_INST_WRITE_WAIT();

  /* Write cmd: TXFIFOWR */
  RF1AINSTRB = 0x7F;

  do
  {
    /* Wait for radio to be ready to accept the data */
    MRFI_RADIO_DATA_WRITE_WAIT();

    /* Write one byte to FIFO */
    RF1ADINB   = *pData;

    pData++;
    len--;

  }while(len);

  /* Allow access to Radio IF */
  MRFI_RIF_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          mrfiRadioInterfaceReadRxFifo
 *
 * @brief       Read data from radio receive FIFO.
 *
 * @param       pData - pointer for storing read data
 * @param       len   - length of data in bytes
 *
 * @return      none
 **************************************************************************************************
 */
uint8_t method = 2;
void mrfiRadioInterfaceReadRxFifo(uint8_t * pData, uint8_t len)
{
  mrfiRIFIState_t s;

  MRFI_RIF_ASSERT(len != 0); /* zero length is not allowed */

  /* Lock out access to Radio IF */
  MRFI_RIF_ENTER_CRITICAL_SECTION(s);

  if(method == 1)
  {
    /* Wait for radio to be ready for next instruction */
    MRFI_RADIO_INST_WRITE_WAIT();

    /* Write cmd: RXFIFORD */
    RF1AINSTRB = 0xFF;

    do
    {
      /* dummy write */
      RF1ADINB = 0;

      /* Wait for data to be available for reading */
      MRFI_RADIO_DATA_READ_WAIT();

      /* Read one byte from FIFO */
      *pData = RF1ADOUT0B;

      pData++;
      len--;

    }while(len);
  }

  if(method == 2)
  {
    do
    {
      /* Wait for radio to be ready for next instruction */
      MRFI_RADIO_INST_WRITE_WAIT();

      /* Write cmd: SNGLRXRD */
      RF1AINSTR1B = 0xBF;

      /* Read byte from FIFO */
      *pData  = RF1ADOUT1B; //auto read register

      pData++;
      len--;

    }while(len);
  }

  /* Allow access to Radio IF */
  MRFI_RIF_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
*/
