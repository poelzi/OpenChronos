/**************************************************************************************************
  Filename:       nwk_security.c
  Revised:        $Date: 2009-01-20 14:05:46 -0800 (Tue, 20 Jan 2009) $
  Revision:       $Revision: 18816 $
  Author:         $Author: lfriedman $

  Description:    This file supports the SimpliciTI Security network application.

  Copyright 2008-2009 Texas Instruments Incorporated. All rights reserved.

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

#include <string.h>     /* needed for NULL */
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_security.h"
#include "nwk_frame.h"
#include "nwk.h"

#ifdef SMPL_SECURE

/*                   *** GENERAL SECURITY OUTLINE ***
 *
 * We are using XTEA (eXtended Tiny Encryption Algorithm) with a fixed
 * number of rounds (32). We have removed the parameters from the API
 * we harvested from the public domain.
 *
 * We are using a CTR-like mode. We use the 64-bit block cipher function of the
 * XTEA code to encipher a concatenation of the 32-bit initialization vector and
 * a 32-bit counter that increments each block. We encrypt using a fixed 128-bit
 * key. The resulting 64-bit output is XOR'ed with the message. If the message is
 * longer than 64 bits we encipher the next block (incrementing the counter) and
 * continue until the message is exhausted. If the last cipher block is longer
 * than the message we simply discard the remaining cipher block.
 */


/******************************************************************************
 * MACROS
 */

/******************************************************************************
 * CONSTANTS AND DEFINES
 */

/* The counter can be off by quite a bit because the number of cipher
 * blocks can easily be more than 1 per frame. Value limited to a
 * maximum of 255.
 */
#define CTR_WINDOW  255

#if (CTR_WINDOW > 255) || (CTR_WINDOW < 0)
#error ERROR: 0 <= CTR_WINDOW < 256
#endif

/* Number of rounds for XTEA algorithm. A parameter in the public domain code
 * but we fix it here at 32.
 */
#define NUM_ROUNDS  32

/* Key and cipher block size constants */
#define SMPL_KEYSIZE_BYTES    16
#define SMPL_KEYSIZE_LONGS     4

/******************************************************************************
 * TYPEDEFS
 */
/* Union used to access key as both a string and as unsigned longs */
typedef union
{
  uint8_t  keyS[SMPL_KEYSIZE_BYTES];
  uint32_t keyL[SMPL_KEYSIZE_LONGS];
} key_t;


/******************************************************************************
 * LOCAL VARIABLES
 */
/* 32-bit Initialization vector */
static uint32_t const sIV = 0x87654321;

/* 128-bit (16 byte) key. Initialized as string but fetched and used in XTEA
 * encryption as 4 unsigned longs. Endianess could count if the peers are on
 * two different MCUs. Endianess is rectified in initialization code.
 *
 * Initialization _MUST_ be done as a string (or character array). Though it
 * won't matter how the initialization is done if both peers are the same
 * endianness, good prectice will initialize these as a string (or character
 * array) so that the endianess reconciliation works properly for all cases.
 */
static key_t sKey = {"SimpliciTI's Key"};

/* Constant set as an authentication code. Note that since it is a
 * fixed value as opposed to a hash of the message it does not provide
 * an integrity check. It will only differentiate two message encryptions
 * with the same LSB but different MSB components. Thus it helps guard
 * against replays.
 */
static secMAC_t const sMAC = 0xA5;

/* This is the 64-bit cipher block target. It is this 64-bit block that
 * is XOR'ed with the actual message to be encrypted.
 */
static uint32_t sMsg[2] = {0, 0};

/******************************************************************************
 * LOCAL FUNCTIONS
 */
static secFCS_t calcFCS(uint8_t *, uint8_t);
static void     msg_encipher(uint8_t *, uint8_t, uint32_t *);
static void     msg_decipher(uint8_t *, uint8_t, uint32_t *);
static void     xtea_encipher(void);

#endif  /* SMPL_SECURE */

/******************************************************************************
 * @fn          nwk_securityInit
 *
 * @brief       Initialize Security network application.
 *
 * input parameters
 *
 * output parameters
 *
 * @return      void
 */
void nwk_securityInit(void)
{
#ifdef SMPL_SECURE
  uint8_t  i;

  /* The key is set as a string. But the XTEA routines operate on 32-bit
   * unsigned longs. Endianess should be taken into account and we do that
   * here by treating the key as being an array of unsigned longs in
   * network order.
   */
  for (i=0; i<sizeof(sKey.keyL)/sizeof(uint32_t); ++i)
  {
    sKey.keyL[i] = ntohl(sKey.keyL[i]);
  }

#endif  /* SMPL_SECURE */
  return;
}

/******************************************************************************
 * @fn          nwk_processSecurity
 *
 * @brief       Security network application frame handler.
 *
 * input parameters
 * @param   frame   - pointer to frame in question
 *
 * output parameters
 *
 * @return    Keep frame for application, release frame, or replay frame.
 */
fhStatus_t nwk_processSecurity(mrfiPacket_t *frame)
{
  return FHS_RELEASE;
}

/******************************************************************************
 * @fn          msg_encipher
 *
 * @brief       Encipher a message using the XTEA algorithm and the modified
 *              CTR mode method.
 *
 * input parameters
 * @param   msg      - pointer to message to encipher
 * @param   len      - length of message
 * @param   cntStart - pointer to the counter used in the cipher block.
 *
 * output parameters
 * @param   cntStart - counter is updated during encryption.
 *
 * @return      void
 */
#ifdef SMPL_SECURE
static void msg_encipher(uint8_t *msg, uint8_t len, uint32_t *cntStart)
{
  uint8_t  i, idx, done;
  uint8_t *mptr = (uint8_t *)&sMsg[0];
  uint32_t ctr;

  if ((NULL == msg) || !len)
  {
    return;
  }

  /* set local counter from input */
  ctr = *cntStart;

  idx  = 0;
  done = 0;
  do
  {
    /* Set block to be enciphered. 1st 32 bits are the IV. The second
     * 32 bits are the current CTR value.
     */
    sMsg[0] = sIV;
    sMsg[1] = ctr;
    /* encrypt */
    xtea_encipher();
    /* increment counter for next time. */
    ctr++;
    /* XOR ciphered block with message to be sent. Only operate
     * up to and including the last message byte which may not
     * be on a cipher block boundary (64 bits == 8 bytes).
     */
    for (i=0; i<sizeof(sMsg) && idx<len; ++i, ++idx)
    {
      msg[idx] ^= mptr[i];
    }

    if (idx >= len)
    {
      /* we're done */
      done = 1;
    }
  } while (!done);

  /* return counter value start for next time */
  *cntStart = ctr;

  return;
}

/******************************************************************************
 * @fn          msg_decipher
 *
 * @brief       Decipher a message using the XTEA algorithm and the modified
 *              CTR mode method.
 *
 * input parameters
 * @param   msg      - pointer to message to decipher
 * @param   len      - length of message
 * @param   cntStart - pointer to the counter used in the cipher block.
 *
 * output parameters
 * @param   cntStart - counter is updated during decryption.
 *
 * @return      void
 */
static void msg_decipher(uint8_t *msg, uint8_t len, uint32_t *cntStart)
{
  msg_encipher(msg, len, cntStart);

  return;
}

/******************************************************************************
 * @fn          xtea_encipher
 *
 * @brief       XTEA encipher algorithm. Calling arguments removed from public
 *              domain code and static-scope values used instead.
 *
 * input parameters
 *
 * output parameters
 *
 * @return      void
 */
void xtea_encipher(void)
{
  uint32_t v0=sMsg[0], v1=sMsg[1];
  uint16_t i;
  uint32_t sum=0, delta=0x9E3779B9;

  for(i=0; i<NUM_ROUNDS; i++)
  {
    v0  += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + sKey.keyL[sum & 3]);
    sum += delta;
    v1  += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + sKey.keyL[(sum>>11) & 3]);
  }

  sMsg[0]=v0;
  sMsg[1]=v1;
}

/******************************************************************************
 * @fn          nwk_setSecureFrame
 *
 * @brief       Called from NWK to secure a frame.
 *
 * input parameters
 * @param   frame   - pointer to frame to secure
 * @param   msglen  - length of message
 * @param   ctr     - pointer to the counter used in the cipher block. This will
 *                    be NULL if a network application is sending a frame. Since
 *                    these are not connection-based there is no counter sync
 *                    issue but we still need a counter value. A random value
 *                    is used.
 *
 * output parameters
 * @param   cntStart - counter is updated during encryption.
 *
 * @return      void
 */
void nwk_setSecureFrame(mrfiPacket_t *frame, uint8_t msglen, uint32_t *ctr)
{
  uint32_t locCnt;

  /* If an encrypted frame is to be sent to a non-connection based port use a
   * random number as the lsb counter value. In this case only the lsb is used
   * for a counter value during decryption. Not as secure but there are still
   * the 32 bits in the IV.
   */
  locCnt = ctr ? *ctr : MRFI_RandomByte();

  /* place counter value into frame */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(frame), F_SEC_CTR_OS, (uint8_t)(locCnt & 0xFF));

  /* Put MAC value in */
  nwk_putNumObjectIntoMsg((void *)&sMAC, (void *)(MRFI_P_PAYLOAD(frame)+F_SEC_MAC_OS), sizeof(secMAC_t));

  /* Put FCS value in */
  {
    secFCS_t fcs = calcFCS(MRFI_P_PAYLOAD(frame)+F_SEC_MAC_OS, msglen+sizeof(secMAC_t));

    nwk_putNumObjectIntoMsg((void *)&fcs, (void *)(MRFI_P_PAYLOAD(frame)+F_SEC_ICHK_OS), sizeof(secFCS_t));
  }

  /* Encrypt frame */
  msg_encipher(MRFI_P_PAYLOAD(frame)+F_SEC_ICHK_OS, msglen+sizeof(secMAC_t)+sizeof(secFCS_t), &locCnt);

  /* Set the Encryption bit */
  PUT_INTO_FRAME(MRFI_P_PAYLOAD(frame), F_ENCRYPT_OS, F_ENCRYPT_OS_MSK);

  /* Update the counter if it was a "real" counter. */
  if (ctr)
  {
    *ctr = locCnt;
  }

  return;
}

/******************************************************************************
 * @fn          calcFCS
 *
 * @brief       Calculate the frame check sequence. Currently it's just a
 *              cumulative XOR of each byte starting with the MAC byte. The
 *              FCS is placed in front of the MAC after the counter hint and is
 *              included in the encryption.
 *
 * input parameters
 * @param   msg      - pointer to message
 * @param   len      - length of message
 *
 * output parameters
 *
 * @return      Returns the FCS using the typedef.
 */
static secFCS_t calcFCS(uint8_t *msg, uint8_t len)
{
  uint8_t  i;
  secFCS_t result = 0;

  for (i=0; i<len; ++i)
  {
    result ^= *(msg+i);
  }

  return result;
}

/******************************************************************************
 * @fn          nwk_getSecureFrame
 *
 * @brief       Called from NWK to get a secure a frame and decrypt.
 *
 * input parameters
 * @param   frame    - pointer to frame containing encrypted message
 * @param   msglen   - length of message
 * @param   ctr      - pointer to the counter used in the cipher block. This will
 *                     be NULL if a network applicaiton is getting a frame. Since
 *                     these are not connection-nbased there is no counter sync
 *                     issue but we still need a counter value.
 *
 * output parameters
 * @param   cntStart - counter is updated during decryption. If decryption fails
 *                     this value is not changed.
 *
 * @return      Returns non-zero if frame decryption is valid, otherwise returns 0.
 */
uint8_t nwk_getSecureFrame(mrfiPacket_t *frame, uint8_t msglen, uint32_t *ctr)
{
  uint8_t  rc = 1;
  uint8_t  done = 0;
  uint8_t  cntHint = GET_FROM_FRAME(MRFI_P_PAYLOAD(frame), F_SEC_CTR_OS);
  uint32_t locCnt, frameCnt;

  /* Construct proposed CTR values */

  /* Just like encryption, we may be talking to a non-connection based
   * peer in which case the counter value is represented by the lsb byte
   * conveyed in the frame.
   */
  locCnt = ctr ? *ctr : cntHint;

  frameCnt = (locCnt & 0xFFFFFF00) + cntHint;

  do
  {
    /* See if counters match */
    if (locCnt == frameCnt)
    {
      /* When the counters appear to match is the only time we actually decipher
       * the message. It is the only time we can do so since out-of-sync lsb counter
       * values guarantees that something is wrong somewhere. Decryption is successful
       * only if the MAC and FCS values match. The message is left as-is after the
       * decipher attempt. Either it appears valid or is doesn't and is discarded.
       * There is no recovery attempt if the counters match but the MAC or FCS do
       * not. It is considered a rogue message.
       */
      msg_decipher(MRFI_P_PAYLOAD(frame)+F_SEC_ICHK_OS, msglen-1, &locCnt);

      /* Get MAC and make sure it matches. A failure can occur if a replayed frame happens
       * to have the correct counter sync value but was encoded with the wrong complete
       * counter value. Otherwise the MAC values must match when the counter values are equal.
       */
      {
        secMAC_t mac;

        nwk_getNumObjectFromMsg((void *)(MRFI_P_PAYLOAD(frame)+F_SEC_MAC_OS), (void *)&mac, sizeof(secMAC_t));
        if (mac != sMAC)
        {
          rc = 0;
        }
      }

      /* FCS check... */
      {
        secFCS_t fcs;

        nwk_getNumObjectFromMsg((void *)(MRFI_P_PAYLOAD(frame)+F_SEC_ICHK_OS), (void *)&fcs, sizeof(secFCS_t));
        if (fcs != calcFCS(MRFI_P_PAYLOAD(frame)+F_SEC_MAC_OS, msglen-1-sizeof(secMAC_t)))
        {
          rc = 0;
        }
      }

      /* we're done. */
      done = 1;
    }
    else
    {
      /* Uh oh. Counters don't match. Try and resync. We need to distinguish among
       * missed frames, duplicates and rogues plus account for counter wrap.
       */
      if (frameCnt > locCnt)
      {
        /* frameCnt is bigger. Second part of test below takes care of
         * the unlikely case of a complete counter wrap (msb's all 0) in
         * which case the test will incorrectly fail when the count is
         * actually within the (wrapped) window. #ifdef'ed to avoid compiler
         * warning in case user sets CNT_WINDOW to 0 (pointless comparison of
         * unsigned value).
         */
        if (((frameCnt-CTR_WINDOW) <= locCnt)
#if CTR_WINDOW > 0
            || (frameCnt < CTR_WINDOW)
#endif
           )
        {
          /* Value within window. We probably missed something. Adjust and decipher.
           * If locCnt is less because it wrapped and frameCnt didn't it means that
           * it's a duplicate or late frame. In that case the following will lead to
           * a decryption that fails sanity checks which is OK because the frame will
           * be correctly rejected.
           */
          locCnt = frameCnt;
        }
        else
        {
          /* It's either a rogue or a really old duplicate packet. In either case
           * we dismiss the frame.
           */
          rc   = 0;
          done = 1;
        }
      }
      else
      {
        /* locCnt is bigger. The only way the frame can be valid is if the
         * counter wrapped causing frameCnt to appear to be smaller. Wrap the
         * counter and decrypt. If the frame isn't valid, i.e., it's late,
         * a duplicate, or a rogue, the decryption will fail sanity checks and
         * the frame will be correctly rejected. The following arithmetic works
         * correctly without a special test for the complete counter wrap case.
         */
        frameCnt += 0x100;   /* wrap the hint-based counter */
        if (((frameCnt-CTR_WINDOW) <= locCnt))
        {
          /* An lsb wrap but still within window. We probably missed something.
           * Adjust (with wrap) and decrypt.
           */
          locCnt = frameCnt;
        }
        else
        {
          /* rogue frame */
          rc   = 0;
          done = 1;
        }
      }
    }
  } while (!done);

  if (ctr && rc)
  {
    /* Only update the counter if the count was a "real" one and the
     * decryption succeeded.
     */
    *ctr = locCnt;
  }

  return rc;
}

#endif  /* SMPL_SECURE */
