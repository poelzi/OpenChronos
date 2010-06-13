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
 *   Primary code file for supported radios.
 * ~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */
#include <string.h>
#include "mrfi.h"
#include "bsp.h"
#include "bsp_macros.h"
#include "bsp_external/mrfi_board_defs.h"
#include "mrfi_defs.h"
#include "mrfi_radio_interface.h"
#include "smartrf/CC430/smartrf_CC430.h"

/* ------------------------------------------------------------------------------------------------
 *                                    Global Constants
 * ------------------------------------------------------------------------------------------------
 */
const uint8_t mrfiBroadcastAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF };

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(MRFI_ADDR_SIZE == ((sizeof(mrfiBroadcastAddr)/sizeof(mrfiBroadcastAddr[0])) * sizeof(mrfiBroadcastAddr[0])));

/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */
#if (defined MRFI_CC430)
  #define MRFI_RSSI_OFFSET    74   /* no units */
#else
  #error "ERROR: RSSI offset value not defined for this radio"
#endif

#define MRFI_LENGTH_FIELD_OFS               __mrfi_LENGTH_FIELD_OFS__
#define MRFI_LENGTH_FIELD_SIZE              __mrfi_LENGTH_FIELD_SIZE__
#define MRFI_HEADER_SIZE                    __mrfi_HEADER_SIZE__
#define MRFI_FRAME_BODY_OFS                 __mrfi_DST_ADDR_OFS__
#define MRFI_BACKOFF_PERIOD_USECS           __mrfi_BACKOFF_PERIOD_USECS__

#define MRFI_RANDOM_OFFSET                   67
#define MRFI_RANDOM_MULTIPLIER              109
#define MRFI_MIN_SMPL_FRAME_SIZE            (MRFI_HEADER_SIZE + NWK_HDR_SIZE)

/* rx metrics definitions, known as appended "packet status bytes" in datasheet parlance */
#define MRFI_RX_METRICS_CRC_OK_MASK         __mrfi_RX_METRICS_CRC_OK_MASK__
#define MRFI_RX_METRICS_LQI_MASK            __mrfi_RX_METRICS_LQI_MASK__


/* ---------- Radio Abstraction ---------- */

#define MRFI_RADIO_PARTNUM          0x00
#define MRFI_RADIO_VERSION          0x06

/* GDO0 == PA_PD signal */
#define MRFI_SETTING_IOCFG0     27

/* GDO1 == RSSI_VALID signal */
#define MRFI_SETTING_IOCFG1     30

/* Main Radio Control State Machine control configuration:
 * Auto Calibrate - when going from IDLE to RX/TX
 * XOSC is OFF in Sleep state.
 */
#define MRFI_SETTING_MCSM0      (0x10)

/* Main Radio Control State Machine control configuration:
 * - Remain RX state after RX
 * - Go to IDLE after TX
 * - RSSI below threshold and NOT receiving.
 */
#define MRFI_SETTING_MCSM1      0x3C

/*
 *  Packet Length - Setting for maximum allowed packet length.
 *  The PKTLEN setting does not include the length field but maximum frame size does.
 *  Subtract length field size from maximum frame size to get value for PKTLEN.
 */
#define MRFI_SETTING_PKTLEN     (MRFI_MAX_FRAME_SIZE - MRFI_LENGTH_FIELD_SIZE)

/* Packet automation control - Original value except WHITE_DATA is extracted from SmartRF setting. */
#define MRFI_SETTING_PKTCTRL0   (0x05 | (SMARTRF_SETTING_PKTCTRL0 & BV(6)))

/* FIFO threshold - this register has fields that need to be configured for the CC1101 */
#define MRFI_SETTING_FIFOTHR    (0x07 | (SMARTRF_SETTING_FIFOTHR & (BV(4)|BV(5)|BV(6)|BV(7))))

/* Max time we can be in a critical section within the delay function.
 * This could be fine-tuned by observing the overhead is calling the bsp delay
 * function. The overhead should be very small compared to this value.
 */
#define MRFI_MAX_DELAY_US 16 /* usec */

/* Packet automation control - base value is power up value whick has APPEND_STATUS enabled; no CRC autoflush */
#define PKTCTRL1_BASE_VALUE         BV(2)
#define PKTCTRL1_ADDR_FILTER_OFF    PKTCTRL1_BASE_VALUE
#define PKTCTRL1_ADDR_FILTER_ON     (PKTCTRL1_BASE_VALUE | (BV(0)|BV(1)))

#ifdef MRFI_ASSERTS_ARE_ON
#define RX_FILTER_ADDR_INITIAL_VALUE  0xFF
#endif

  /* The SW timer is calibrated by adjusting the call to the microsecond delay
   * routine. This allows maximum calibration control with repects to the longer
   * times requested by applicationsd and decouples internal from external calls
   * to the microsecond routine which can be calibrated independently.
   */
#if defined(SW_TIMER)
#define APP_USEC_VALUE    1000
#else
#define APP_USEC_VALUE    1000
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Macros
 * ------------------------------------------------------------------------------------------------
 */
#define MRFI_ENABLE_SYNC_PIN_INT()                  (RF1AIE |= BV(9))
#define MRFI_DISABLE_SYNC_PIN_INT()                 (RF1AIE &= ~BV(9))
#define MRFI_CLEAR_SYNC_PIN_INT_FLAG()              (RF1AIFG &= ~BV(9))
#define MRFI_SYNC_PIN_INT_IS_ENABLED()              (RF1AIE & BV(9))
#define MRFI_SYNC_PIN_IS_HIGH()                     (RF1AIN & BV(9))
#define MRFI_SYNC_PIN_INT_FLAG_IS_SET()             (RF1AIFG & BV(9))


#define MRFI_CLEAR_PAPD_PIN_INT_FLAG()              (RF1AIFG &= ~BV(0))
#define MRFI_PAPD_PIN_IS_HIGH()                     (RF1AIN & BV(0))
#define MRFI_PAPD_INT_FLAG_IS_SET()                 (RF1AIFG & BV(0))


/* RSSI valid signal is available on the GDO_1 */
#define MRFI_RSSI_VALID_WAIT()  while(!(RF1AIN & BV(1)));


/* Abstract radio interface calls. Could use these later to
 * merge code from similar radio but different interface.
 */
#define MRFI_STROBE(cmd)                      mrfiRadioInterfaceCmdStrobe(cmd)
#define MRFI_RADIO_REG_READ(reg)              mrfiRadioInterfaceReadReg(reg)
#define MRFI_RADIO_REG_WRITE(reg, value)      mrfiRadioInterfaceWriteReg(reg, value)
#define MRFI_RADIO_WRITE_TX_FIFO(pData, len)  mrfiRadioInterfaceWriteTxFifo(pData, len)
#define MRFI_RADIO_READ_RX_FIFO(pData, len)   mrfiRadioInterfaceReadRxFifo(pData, len)


#define MRFI_STROBE_IDLE_AND_WAIT()              \
{                                                \
  MRFI_STROBE( SIDLE );                          \
  /* Wait for XOSC to be stable and radio in IDLE state */ \
  while (MRFI_STROBE( SNOP ) & 0xF0) ;           \
}


/* ------------------------------------------------------------------------------------------------
 *                                    Local Constants
 * ------------------------------------------------------------------------------------------------
 */
const uint8_t mrfiRadioCfg[][2] =
{
  /* internal radio configuration */
  {  IOCFG0,    MRFI_SETTING_IOCFG0       }, /* Configure GDO_0 to output PA_PD signal (low during TX, high otherwise). */
  {  IOCFG1,    MRFI_SETTING_IOCFG1       }, /* Configure GDO_1 to output RSSI_VALID signal (high when RSSI is valid, low otherwise). */
  {  MCSM1,     MRFI_SETTING_MCSM1        }, /* CCA mode, RX_OFF_MODE and TX_OFF_MODE */
  {  MCSM0,     MRFI_SETTING_MCSM0        }, /* AUTO_CAL and XOSC state in sleep */
  {  PKTLEN,    MRFI_SETTING_PKTLEN       },
  {  PKTCTRL0,  MRFI_SETTING_PKTCTRL0     },
  {  FIFOTHR,   MRFI_SETTING_FIFOTHR      },

/* imported SmartRF radio configuration */

  {  FSCTRL1,   SMARTRF_SETTING_FSCTRL1   },
  {  FSCTRL0,   SMARTRF_SETTING_FSCTRL0   },
  {  FREQ2,     SMARTRF_SETTING_FREQ2     },
  {  FREQ1,     SMARTRF_SETTING_FREQ1     },
  {  FREQ0,     SMARTRF_SETTING_FREQ0     },
  {  MDMCFG4,   SMARTRF_SETTING_MDMCFG4   },
  {  MDMCFG3,   SMARTRF_SETTING_MDMCFG3   },
  {  MDMCFG2,   SMARTRF_SETTING_MDMCFG2   },
  {  MDMCFG1,   SMARTRF_SETTING_MDMCFG1   },
  {  MDMCFG0,   SMARTRF_SETTING_MDMCFG0   },
  {  DEVIATN,   SMARTRF_SETTING_DEVIATN   },
  {  FOCCFG,    SMARTRF_SETTING_FOCCFG    },
  {  BSCFG,     SMARTRF_SETTING_BSCFG     },
  {  AGCCTRL2,  SMARTRF_SETTING_AGCCTRL2  },
  {  AGCCTRL1,  SMARTRF_SETTING_AGCCTRL1  },
  {  AGCCTRL0,  SMARTRF_SETTING_AGCCTRL0  },
  {  FREND1,    SMARTRF_SETTING_FREND1    },
  {  FREND0,    SMARTRF_SETTING_FREND0    },
  {  FSCAL3,    SMARTRF_SETTING_FSCAL3    },
  {  FSCAL2,    SMARTRF_SETTING_FSCAL2    },
  {  FSCAL1,    SMARTRF_SETTING_FSCAL1    },
  {  FSCAL0,    SMARTRF_SETTING_FSCAL0    },
  {  TEST2,     SMARTRF_SETTING_TEST2     },
  {  TEST1,     SMARTRF_SETTING_TEST1     },
  {  TEST0,     SMARTRF_SETTING_TEST0     },
};


/*
 *  Logical channel table - this table translates logical channel into
 *  actual radio channel number.  Channel 0, the default channel, is
 *  determined by the channel exported from SmartRF Studio.  The other
 *  table entries are derived from that default.  Each derived channel is
 *  masked with 0xFF to prevent generation of an illegal channel number.
 *
 *  This table is easily customized.  Just replace or add entries as needed.
 *  If the number of entries changes, the corresponding #define must also
 *  be adjusted.  It is located in mrfi_defs.h and is called __mrfi_NUM_LOGICAL_CHANS__.
 *  The static assert below ensures that there is no mismatch.
 */
// [BM] Changed channel assignment to comply with local regulations
#ifdef ISM_EU 
static const uint8_t mrfiLogicalChanTable[] =
{
  0,
  50,
  80,
  110
};
#else
	#ifdef ISM_US
	static const uint8_t mrfiLogicalChanTable[] =
	{
	  20,
	  50,
	  80,
	  110
	};
	#else
		#ifdef ISM_LF
		static const uint8_t mrfiLogicalChanTable[] =
		{
		  0,
		  50,
		  80,
		  110
		};		
		#else
			#error "Wrong ISM band specified (valid are ISM_LF, ISM_EU and ISM_US)"
		#endif
	#endif
#endif
//static const uint8_t mrfiLogicalChanTable[] =
//{
//  SMARTRF_SETTING_CHANNR,
//  50,
//  80,
//  110
//};
/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(__mrfi_NUM_LOGICAL_CHANS__ == ((sizeof(mrfiLogicalChanTable)/sizeof(mrfiLogicalChanTable[0])) * sizeof(mrfiLogicalChanTable[0])));

/*
 *  RF Power setting table - this table translates logical power value
 *  to radio register setting.  The logical power value is used directly
 *  as an index into the power setting table. The values in the table are
 *  from low to high. The default settings set 3 values: -20 dBm, -10 dBm,
 *  and 0 dBm. The default at startup is the highest value. Note that these
 *  are approximate depending on the radio. Information is taken from the
 *  data sheet.
 *
 *  This table is easily customized.  Just replace or add entries as needed.
 *  If the number of entries changes, the corresponding #define must also
 *  be adjusted.  It is located in mrfi_defs.h and is called __mrfi_NUM_POWER_SETTINGS__.
 *  The static assert below ensures that there is no mismatch.
 *
 * For the CC430 use the CC1100 values.
 */
static const uint8_t mrfiRFPowerTable[] =
{
// [BM] Changed default output power to comply with dongle settings
  0x0F,
  0x27,
// [BM] Increase output power from -0.3dBm to +1.4dBm (433MHz) / +1.1dBm (868MHz) / +1.3dBm (915MHz) to compensate antenna loss  
#ifdef ISM_EU 
  0x8C
#else
  #ifdef ISM_US
    0x8B
  #else
  	#ifdef ISM_LF
  		0x8D
  	#else
	    #error "Wrong ISM band specified (valid are ISM_LF, ISM_EU and ISM_US)"
    #endif
  #endif
#endif
};
//static const uint8_t mrfiRFPowerTable[] =
//{
//  0x0D,
//  0x34,
//  0x8E
//};

/* verify number of table entries matches the corresponding #define */
BSP_STATIC_ASSERT(__mrfi_NUM_POWER_SETTINGS__ == ((sizeof(mrfiRFPowerTable)/sizeof(mrfiRFPowerTable[0])) * sizeof(mrfiRFPowerTable[0])));

/* ------------------------------------------------------------------------------------------------
 *                                       Local Prototypes
 * ------------------------------------------------------------------------------------------------
 */
static void Mrfi_SyncPinRxIsr(void);
static void Mrfi_RxModeOn(void);
static void Mrfi_RandomBackoffDelay(void);
static void Mrfi_RxModeOff(void);
static void Mrfi_DelayUsec(uint16_t howLong);
static void Mrfi_DelayUsecSem(uint16_t howLong);
static int8_t Mrfi_CalculateRssi(uint8_t rawValue);
static uint8_t Mrfi_RxAddrIsFiltered(uint8_t * pAddr);


/* ------------------------------------------------------------------------------------------------
 *                                       Local Variables
 * ------------------------------------------------------------------------------------------------
 */
static uint8_t mrfiRadioState  = MRFI_RADIO_STATE_UNKNOWN;
static mrfiPacket_t mrfiIncomingPacket;
static uint8_t mrfiRndSeed = 0;

/* reply delay support */
static volatile uint8_t  sKillSem = 0;
static volatile uint8_t  sReplyDelayContext = 0;
static          uint16_t sReplyDelayScalar = 0;
static          uint16_t sBackoffHelper = 0;

static uint8_t mrfiRxFilterEnabled = 0;
static uint8_t mrfiRxFilterAddr[MRFI_ADDR_SIZE] = { RX_FILTER_ADDR_INITIAL_VALUE };

/* These counters are only for diagnostic purpose */
static uint32_t crcFail = 0;
static uint32_t crcPass = 0;
static uint32_t noFrame = 0;

// [BM] Radio frequency offset read from calibration memory
// Compensates crystal deviation from 26MHz nominal value
extern unsigned char rf_frequoffset;

/**************************************************************************************************
 * @fn          MRFI_Init
 *
 * @brief       Initialize MRFI.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Init(void)
{
  /* ------------------------------------------------------------------
   *    Radio power-up reset
   *   ----------------------
   */
  memset(&mrfiIncomingPacket, 0x0, sizeof(mrfiIncomingPacket));

  /* Initialize the radio interface */
  mrfiRadioInterfaceInit();

  /* Strobe Reset: Resets the radio and puts it in SLEEP state. */
  MRFI_STROBE( SRES );

  /* verify the correct radio is installed */
  MRFI_ASSERT( MRFI_RADIO_REG_READ( PARTNUM ) == MRFI_RADIO_PARTNUM );  /* incorrect radio specified */
  MRFI_ASSERT( MRFI_RADIO_REG_READ( VERSION ) == MRFI_RADIO_VERSION );  /* incorrect radio specified  */

  /* Put radio in Idle state */
  MRFI_STROBE_IDLE_AND_WAIT();


  /* ------------------------------------------------------------------
   *    Configure radio
   *   -----------------
   */

  /* Configure Radio interrupts:
   *
   * RF1AIN_0 => Programmed to PA_PD signal.
   *             Configure it to interrupt on falling edge.
   *
   * RF1AIN_1 => Programmed to RSSI Valid signal.
   *             No need to configure for interrupt. This value will be read
   *             through polling.
   *
   * RF1AIN_9 => Rising edge indicates SYNC sent/received and
   *             Falling edge indicates end of packet.
   *             Configure it to interrupt on falling edge.
   */

  /* Select Interrupt edge for PA_PD and SYNC signal:
   * Interrupt Edge select register: 1 == Interrupt on High to Low transition.
   */
  RF1AIES = BV(0) | BV(9);

  /* Write the power output to the PA_TABLE and verify the write operation.  */
  {
    uint8_t      readbackPATableValue = 0;
    bspIState_t  s;

    BSP_ENTER_CRITICAL_SECTION(s);

    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRW = 0x7E51;                    /* PA Table write (burst) */

    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRB = RF_SNOP;                   /* reset pointer */

    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRB = 0xFE;                      /* PA Table read (burst) */

    while( !(RF1AIFCTL1 & RFDINIFG));
    RF1ADINB    = 0x00;                     /* dummy write */

    while( !(RF1AIFCTL1 & RFDOUTIFG));
    readbackPATableValue = RF1ADOUT0B;

    MRFI_ASSERT(readbackPATableValue == 0x51);

    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRB = RF_SNOP;

    BSP_EXIT_CRITICAL_SECTION(s);
  }

  /* initialize radio registers */
  {
    uint8_t i;

    for (i=0; i<(sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])); i++)
    {
      MRFI_RADIO_REG_WRITE(mrfiRadioCfg[i][0], mrfiRadioCfg[i][1]);
    }
  }

  /* Confirm that the values were written correctly.
   */
  {
    uint8_t i;

    for (i=0; i<(sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])); i++)
    {
        MRFI_ASSERT( mrfiRadioCfg[i][1] == MRFI_RADIO_REG_READ(mrfiRadioCfg[i][0]) );
    }
  }

  // [BM] Apply global frequency offset to FSCTRL0
  MRFI_STROBE_IDLE_AND_WAIT();
  MRFI_RADIO_REG_WRITE(FSCTRL0, rf_frequoffset);
  
  /* set default channel */
  MRFI_SetLogicalChannel( 0 );

  /* Set default power level */
  MRFI_SetRFPwr(MRFI_NUM_POWER_SETTINGS- 1);

  /* Generate Random seed:
   * We will use the RSSI value to generate our random seed.
   */

  /* Put the radio in RX state */
  MRFI_STROBE( SRX );

  /* delay for the rssi to be valid */
  MRFI_RSSI_VALID_WAIT();

  {
    uint8_t i;
    for(i=0; i<16; i++)
    {
      /* use most random bit of rssi to populate the random seed */
      mrfiRndSeed = (mrfiRndSeed << 1) | (MRFI_RADIO_REG_READ(RSSI) & 0x01);
    }
  }

  /* Force the seed to be non-zero by setting one bit, just in case... */
  mrfiRndSeed |= 0x0080;

  /* Turn off RF. */
  Mrfi_RxModeOff();

  /* Strobe Power Down (SPWD): puts the radio in SLEEP state. */
  /* Chip bug: Radio does not come out of this SLEEP when put to sleep
   * using the SPWD cmd. However, it does wakes up if SXOFF was used to
   * put it to sleep.
   */
  MRFI_STROBE( SXOFF );

  /* Initial radio state is IDLE state */
  mrfiRadioState = MRFI_RADIO_STATE_OFF;

  /*****************************************************************************************
   *                            Compute reply delay scalar
   *
   * Formula from data sheet for all the narrow band radios is:
   *
   *                (256 + DATAR_Mantissa) * 2^(DATAR_Exponent)
   * DATA_RATE =    ------------------------------------------ * f(xosc)
   *                                    2^28
   *
   * To try and keep some accuracy we change the exponent of the denominator
   * to (28 - (exponent from the configuration register)) so we do a division
   * by a smaller number. We find the power of 2 by shifting.
   *
   * The maximum delay needed depends on the MAX_APP_PAYLOAD parameter. Figure
   * out how many bits that will be when overhead is included. Bits/bits-per-second
   * is seconds to transmit (or receive) the maximum frame. We multiply this number
   * by 1000 to find the time in milliseconds. We then additionally multiply by
   * 10 so we can add 5 and divide by 10 later, thus rounding up to the number of
   * milliseconds. This last won't matter for slow transmissions but for faster ones
   * we want to err on the side of being conservative and making sure the radio is on
   * to receive the reply. The semaphore monitor will shut it down. The delay adds in
   * a fudge factor that includes processing time on peer plus lags in Rx and processing
   * time on receiver's side.
   *
   * Note that we assume a 26 MHz clock for the radio...
   * ***************************************************************************************
   */
#define   MRFI_RADIO_OSC_FREQ         26000000
#define   PHY_PREAMBLE_SYNC_BYTES     8

  {
    uint32_t dataRate, bits;
    uint16_t exponent, mantissa;

    /* mantissa is in MDMCFG3 */
    mantissa = 256 + SMARTRF_SETTING_MDMCFG3;

    /* exponent is lower nibble of MDMCFG4. */
    exponent = 28 - (SMARTRF_SETTING_MDMCFG4 & 0x0F);

    /* we can now get data rate */
    dataRate = mantissa * (MRFI_RADIO_OSC_FREQ>>exponent);

    bits = ((uint32_t)((PHY_PREAMBLE_SYNC_BYTES + MRFI_MAX_FRAME_SIZE)*8))*10000;

    /* processing on the peer + the Tx/Rx time plus more */
    sReplyDelayScalar = PLATFORM_FACTOR_CONSTANT + (((bits/dataRate)+5)/10);

    /* This helper value is used to scale the backoffs during CCA. At very
     * low data rates we need to backoff longer to prevent continual sampling
     * of valid frames which take longer to send at lower rates. Use the scalar
     * we just calculated divided by 32. With the backoff algorithm backing
     * off up to 16 periods this will result in waiting up to about 1/2 the total
     * scalar value. For high data rates this does not contribute at all. Value
     * is in microseconds.
     */
    sBackoffHelper = MRFI_BACKOFF_PERIOD_USECS + (sReplyDelayScalar>>5)*1000;
  }

  /* Clean out buffer to protect against spurious frames */
  memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));
  memset(mrfiIncomingPacket.rxMetrics, 0x00, sizeof(mrfiIncomingPacket.rxMetrics));

  /* enable global interrupts */
  BSP_ENABLE_INTERRUPTS();
}


/**************************************************************************************************
 * @fn          MRFI_Transmit
 *
 * @brief       Transmit a packet using CCA algorithm.
 *
 * @param       pPacket - pointer to packet to transmit
 *
 * @return      Return code indicates success or failure of transmit:
 *                  MRFI_TX_RESULT_SUCCESS - transmit succeeded
 *                  MRFI_TX_RESULT_FAILED  - transmit failed because CCA failed
 **************************************************************************************************
 */
uint8_t MRFI_Transmit(mrfiPacket_t * pPacket, uint8_t txType)
{
  uint8_t ccaRetries;
  uint8_t txBufLen;
  uint8_t returnValue = MRFI_TX_RESULT_SUCCESS;

  /* radio must be awake to transmit */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* Turn off reciever. We can ignore/drop incoming packets during transmit. */
  Mrfi_RxModeOff();

  /* compute number of bytes to write to transmit FIFO */
  txBufLen = pPacket->frame[MRFI_LENGTH_FIELD_OFS] + MRFI_LENGTH_FIELD_SIZE;

  /* ------------------------------------------------------------------
   *    Write packet to transmit FIFO
   *   --------------------------------
   */
  MRFI_RADIO_WRITE_TX_FIFO(&(pPacket->frame[0]), txBufLen);


  /* ------------------------------------------------------------------
   *    Immediate transmit
   *   ---------------------
   */
  if (txType == MRFI_TX_TYPE_FORCED)
  {
    /* Issue the TX strobe. */
    MRFI_STROBE( STX );

    /* Wait for transmit to complete */
    while(!MRFI_SYNC_PIN_INT_FLAG_IS_SET());

    /* Clear the interrupt flag */
    MRFI_CLEAR_SYNC_PIN_INT_FLAG();
  }
  else
  {
    /* ------------------------------------------------------------------
     *    CCA transmit
     *   ---------------
     */

    MRFI_ASSERT( txType == MRFI_TX_TYPE_CCA );

    /* set number of CCA retries */
    ccaRetries = MRFI_CCA_RETRIES;


    /* ===============================================================================
     *    Main Loop
     *  =============
     */
    for (;;)
    {
      /* Radio must be in RX mode for CCA to happen.
       * Otherwise it will transmit without CCA happening.
       */

      /* Can not use the Mrfi_RxModeOn() function here since it turns on the
       * Rx interrupt, which we don't want in this case.
       */
      MRFI_STROBE( SRX );

      /* wait for the rssi to be valid. */
      MRFI_RSSI_VALID_WAIT();

      /*
       *  Clear the PA_PD pin interrupt flag.  This flag, not the interrupt itself,
       *  is used to capture the transition that indicates a transmit was started.
       *  The pin level cannot be used to indicate transmit success as timing may
       *  prevent the transition from being detected.  The interrupt latch captures
       *  the event regardless of timing.
       */
      MRFI_CLEAR_PAPD_PIN_INT_FLAG();

      /* send strobe to initiate transmit */
      MRFI_STROBE( STX );

      /* Delay long enough for the PA_PD signal to indicate a
       * successful transmit. This is the 250 XOSC periods
       * (9.6 us for a 26 MHz crystal).
       * Found out that we need a delay of atleast 25 us on CC1100 to see
       * the PA_PD signal change. Hence keeping the same for CC430
       */
      Mrfi_DelayUsec(25);


      /* PA_PD signal goes from HIGH to LOW when going from RX to TX state.
       * This transition is trapped as a falling edge interrupt flag
       * to indicate that CCA passed and the transmit has started.
       */
      if (MRFI_PAPD_INT_FLAG_IS_SET())
      {
        /* ------------------------------------------------------------------
        *    Clear Channel Assessment passed.
        *   ----------------------------------
        */

        /* Clear the PA_PD int flag */
        MRFI_CLEAR_PAPD_PIN_INT_FLAG();

        /* PA_PD signal stays LOW while in TX state and goes back to HIGH when
         * the radio transitions to RX state.
         */
        /* wait for transmit to complete */
        while (!MRFI_PAPD_PIN_IS_HIGH());

        /* transmit done, break */
        break;
      }
      else
      {
        /* ------------------------------------------------------------------
         *    Clear Channel Assessment failed.
         *   ----------------------------------
         */

        /* Turn off radio and save some power during backoff */

        /* NOTE: Can't use Mrfi_RxModeOff() - since it tries to update the
         * sync signal status which we are not using during the TX operation.
         */
        MRFI_STROBE_IDLE_AND_WAIT();

        /* flush the receive FIFO of any residual data */
        MRFI_STROBE( SFRX );

        /* Retry ? */
        if (ccaRetries != 0)
        {
          /* delay for a random number of backoffs */
          Mrfi_RandomBackoffDelay();

          /* decrement CCA retries before loop continues */
          ccaRetries--;
        }
        else /* No CCA retries are left, abort */
        {
          /* set return value for failed transmit and break */
          returnValue = MRFI_TX_RESULT_FAILED;
          break;
        }
      } /* CCA Failed */
    } /* CCA loop */
  }/* txType is CCA */


  /* Done with TX. Clean up time... */

  /* Radio is already in IDLE state */

  /*
   * Flush the transmit FIFO.  It must be flushed so that
   * the next transmit can start with a clean slate.
   */
  MRFI_STROBE( SFTX );

  /* If the radio was in RX state when transmit was attempted,
   * put it back to Rx On state.
   */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }

  return( returnValue );
}

/**************************************************************************************************
 * @fn          MRFI_Receive
 *
 * @brief       Copies last packet received to the location specified.
 *              This function is meant to be called after the ISR informs
 *              higher level code that there is a newly received packet.
 *
 * @param       pPacket - pointer to location of where to copy received packet
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Receive(mrfiPacket_t * pPacket)
{
  *pPacket = mrfiIncomingPacket;
}

/**************************************************************************************************
 * @fn          Mrfi_SyncPinRxIsr
 *
 * @brief       This interrupt is called when the SYNC signal transition from high to low.
 *              The sync signal is routed to the sync pin which is a GPIO pin.  This high-to-low
 *              transition signifies a receive has completed.  The SYNC signal also goes from
 *              high to low when a transmit completes.   This is protected against within the
 *              transmit function by disabling sync pin interrupts until transmit completes.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_SyncPinRxIsr(void)
{
  uint8_t frameLen = 0x00;
  uint8_t rxBytes;

  /* We should receive this interrupt only in RX state
   * Should never receive it if RX was turned On only for
   * some internal mrfi processing like - during CCA.
   * Otherwise something is terribly wrong.
   */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  /* ------------------------------------------------------------------
   *    Get RXBYTES
   *   -------------
   */

  /*
   *  Read the RXBYTES register from the radio.
   *  Bit description of RXBYTES register:
   *    bit 7     - RXFIFO_OVERFLOW, set if receive overflow occurred
   *    bits 6:0  - NUM_BYTES, number of bytes in receive FIFO
   *
   *  Due a chip bug, the RXBYTES register must read the same value twice
   *  in a row to guarantee an accurate value.
   */
  {
    uint8_t rxBytesVerify;

    rxBytesVerify = MRFI_RADIO_REG_READ( RXBYTES );

    do
    {
      rxBytes = rxBytesVerify;
      rxBytesVerify = MRFI_RADIO_REG_READ( RXBYTES );
    }
    while (rxBytes != rxBytesVerify);
  }


  /* ------------------------------------------------------------------
   *    FIFO empty?
   *   -------------
   */

  /*
   *  See if the receive FIFIO is empty before attempting to read from it.
   *  It is possible nothing the FIFO is empty even though the interrupt fired.
   *  This can happen if address check is enabled and a non-matching packet is
   *  received.  In that case, the radio automatically removes the packet from
   *  the FIFO.
   */
  if (rxBytes == 0)
  {
    /* receive FIFO is empty - do nothing, skip to end */
  }
  else
  {
    /* receive FIFO is not empty, continue processing */

    /* ------------------------------------------------------------------
     *    Process frame length
     *   ----------------------
     */

    /* read the first byte from FIFO - the packet length */
    MRFI_RADIO_READ_RX_FIFO(&frameLen, MRFI_LENGTH_FIELD_SIZE);


    /*
     *  Make sure that the frame length just read corresponds to number of bytes in the buffer.
     *  If these do not match up something is wrong.
     *
     *  This can happen for several reasons:
     *   1) Incoming packet has an incorrect format or is corrupted.
     *   2) The receive FIFO overflowed.  Overflow is indicated by the high
     *      bit of rxBytes.  This guarantees the value of rxBytes value will not
     *      match the number of bytes in the FIFO for overflow condition.
     *   3) Interrupts were blocked for an abnormally long time which
     *      allowed a following packet to at least start filling the
     *      receive FIFO.  In this case, all received and partially received
     *      packets will be lost - the packet in the FIFO and the packet coming in.
     *      This is the price the user pays if they implement a giant
     *      critical section.
     *   4) A failed transmit forced radio to IDLE state to flush the transmit FIFO.
     *      This could cause an active receive to be cut short.
     *
     *  Also check the sanity of the length to guard against rogue frames.
     */
    if ((rxBytes != (frameLen + MRFI_LENGTH_FIELD_SIZE + MRFI_RX_METRICS_SIZE))           ||
        ((frameLen + MRFI_LENGTH_FIELD_SIZE) > MRFI_MAX_FRAME_SIZE) ||
        (frameLen < MRFI_MIN_SMPL_FRAME_SIZE)
       )
    {
      bspIState_t s;
      noFrame++;

      /* mismatch between bytes-in-FIFO and frame length */

      /*
       *  Flush receive FIFO to reset receive.  Must go to IDLE state to do this.
       *  The critical section guarantees a transmit does not occur while cleaning up.
       */
      BSP_ENTER_CRITICAL_SECTION(s);
      MRFI_STROBE_IDLE_AND_WAIT();
      MRFI_STROBE( SFRX );
      MRFI_STROBE( SRX );
      BSP_EXIT_CRITICAL_SECTION(s);

      /* flush complete, skip to end */
    }
    else
    {
      /* bytes-in-FIFO and frame length match up - continue processing */

      /* ------------------------------------------------------------------
       *    Get packet
       *   ------------
       */

      /* clean out buffer to help protect against spurious frames */
      memset(mrfiIncomingPacket.frame, 0x00, sizeof(mrfiIncomingPacket.frame));

      /* set length field */
      mrfiIncomingPacket.frame[MRFI_LENGTH_FIELD_OFS] = frameLen;

      /* get packet from FIFO */
      MRFI_RADIO_READ_RX_FIFO(&(mrfiIncomingPacket.frame[MRFI_FRAME_BODY_OFS]), frameLen);

      /* get receive metrics from FIFO */
      MRFI_RADIO_READ_RX_FIFO(&(mrfiIncomingPacket.rxMetrics[0]), MRFI_RX_METRICS_SIZE);


      /* ------------------------------------------------------------------
       *    CRC check
       *   ------------
       */

      /*
       *  Note!  Automatic CRC check is not, and must not, be enabled.  This feature
       *  flushes the *entire* receive FIFO when CRC fails.  If this feature is
       *  enabled it is possible to be reading from the FIFO and have a second
       *  receive occur that fails CRC and automatically flushes the receive FIFO.
       *  This could cause reads from an empty receive FIFO which puts the radio
       *  into an undefined state.
       */

      /* determine if CRC failed */
      if (!(mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] & MRFI_RX_METRICS_CRC_OK_MASK))
      {
        /* CRC failed - do nothing, skip to end */
        crcFail++;
      }
      else
      {
        /* CRC passed - continue processing */
        crcPass++;

        /* ------------------------------------------------------------------
         *    Filtering
         *   -----------
         */

        /* if address is not filtered, receive is successful */
        if (!Mrfi_RxAddrIsFiltered(MRFI_P_DST_ADDR(&mrfiIncomingPacket)))
        {
          {
            /* ------------------------------------------------------------------
             *    Receive successful
             *   --------------------
             */

            /* Convert the raw RSSI value and do offset compensation for this radio */
            mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_RSSI_OFS] =
                Mrfi_CalculateRssi(mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_RSSI_OFS]);

            /* Remove the CRC valid bit from the LQI byte */
            mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] =
              (mrfiIncomingPacket.rxMetrics[MRFI_RX_METRICS_CRC_LQI_OFS] & MRFI_RX_METRICS_LQI_MASK);


            /* call external, higher level "receive complete" processing routine */
            MRFI_RxCompleteISR();
          }
        }
      }
    }
  }

  /* ------------------------------------------------------------------
   *    End of function
   *   -------------------
   */
}

/**************************************************************************************************
 * @fn          Mrfi_RxModeOn
 *
 * @brief       Put radio into receive mode.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RxModeOn(void)
{
  /* clear any residual receive interrupt */
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();

  /* send strobe to enter receive mode */
  MRFI_STROBE( SRX );

  /* enable receive interrupts */
  MRFI_ENABLE_SYNC_PIN_INT();
}

/**************************************************************************************************
 * @fn          MRFI_RxOn
 *
 * @brief       Turn on the receiver.  No harm is done if this function is called when
 *              receiver is already on.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_RxOn(void)
{
  /* radio must be awake before we can move it to RX state */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* if radio is off, turn it on */
  if(mrfiRadioState != MRFI_RADIO_STATE_RX)
  {
    mrfiRadioState = MRFI_RADIO_STATE_RX;
    Mrfi_RxModeOn();
  }
}

/**************************************************************************************************
 * @fn          Mrfi_RxModeOff
 *
 * @brief       -
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RxModeOff(void)
{
  /* disable receive interrupts */
  MRFI_DISABLE_SYNC_PIN_INT();

  /* turn off radio */
  MRFI_STROBE_IDLE_AND_WAIT();

  /* flush the receive FIFO of any residual data */
  MRFI_STROBE( SFRX );

  /* clear receive interrupt */
  MRFI_CLEAR_SYNC_PIN_INT_FLAG();
}


/**************************************************************************************************
 * @fn          MRFI_RxIdle
 *
 * @brief       Put radio in idle mode (receiver if off).  No harm is done this function is
 *              called when radio is already idle.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_RxIdle(void)
{
  /* radio must be awake to move it to idle mode */
  MRFI_ASSERT( mrfiRadioState != MRFI_RADIO_STATE_OFF );

  /* if radio is on, turn it off */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOff();
    mrfiRadioState = MRFI_RADIO_STATE_IDLE;
  }
}


/**************************************************************************************************
 * @fn          MRFI_Sleep
 *
 * @brief       Request radio go to sleep.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_Sleep(void)
{
  bspIState_t s;

  /* Critical section necessary for watertight testing and
   * setting of state variables.
   */
  BSP_ENTER_CRITICAL_SECTION(s);

  /* If radio is not asleep, put it to sleep */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    /* go to idle so radio is in a known state before sleeping */
    MRFI_RxIdle();

    /* Strobe Power Down (SPWD): puts the radio in SLEEP state. */
    /* Chip bug: Radio does not come out of this SLEEP when put to sleep
     * using the SPWD cmd. However, it does wakes up if SXOFF was used to
     * put it to sleep.
     */
    MRFI_STROBE( SXOFF );

    /* Our new state is OFF */
    mrfiRadioState = MRFI_RADIO_STATE_OFF;
  }

  BSP_EXIT_CRITICAL_SECTION(s);
}


/**************************************************************************************************
 * @fn          MRFI_WakeUp
 *
 * @brief       Wake up radio from sleep state.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_WakeUp(void)
{
  /* if radio is already awake, just ignore wakeup request */
  if(mrfiRadioState != MRFI_RADIO_STATE_OFF)
  {
    return;
  }

  MRFI_STROBE_IDLE_AND_WAIT();

  /* enter idle mode */
  mrfiRadioState = MRFI_RADIO_STATE_IDLE;
}


/**************************************************************************************************
 * @fn          MRFI_RadioIsr
 *
 * @brief       Radio Interface interrupts as well as Radio interrupts are
 *              mapped to this interrupt vector.
 *
 * @param       -
 *
 * @return      -
 **************************************************************************************************
 */
// [BM] Changed because CC1101_VECTOR ISR is declared of source code and this handler is called indirect
//BSP_ISR_FUNCTION( MRFI_RadioIsr, CC1101_VECTOR )
void MRFI_RadioIsr(void)
{
  uint16_t coreIntSource = RF1AIV;            /* Radio Core      interrupt register */
  uint16_t interfaceIntSource = RF1AIFIV;     /* Radio Interface interrupt register */

  /* Interface interrupt */
  if(interfaceIntSource)
  {
    if(interfaceIntSource == RF1AIFIV_RFERRIFG)
    {
      uint16_t interfaceError = RF1AIFERRV;

      if(interfaceError == RF1AIFERRV_LVERR)
      {
        /* Low core voltage error */
      }
      else if(interfaceError == RF1AIFERRV_OPERR)
      {
        /* Operand error */
      }
      else if(interfaceError == RF1AIFERRV_OUTERR)
      {
        /* output data not available error */
      }
      else if(interfaceError == RF1AIFERRV_OPOVERR)
      {
        /* Operand overwrite error */
      }
      else
      {
        /* Can't possibly happen. If interface error flag was set,
         * then one of the interface errors must be set.
         */
        MRFI_FORCE_ASSERT();
      }

      /* Assert anyways. No error handling implemented. */
      MRFI_FORCE_ASSERT();
    }
    else
    {
      /* Not expecting any other interface interrupts (data in/out, status/instr in, fifo rx/tx) */
      MRFI_FORCE_ASSERT();
    }
  }

  /* Radio Core interrupt */
  if(coreIntSource)
  {
    /* Check for SYNC interrupt */
    if(coreIntSource == RF1AIV_RFIFG9)
    {
      if(MRFI_SYNC_PIN_INT_IS_ENABLED())
      {
        /*  clear the sync pin interrupt, run sync pin ISR */
        /*
         *  NOTE!  The following macro clears the interrupt flag but it also *must*
         *  reset the interrupt capture.  In other words, if a second interrupt
         *  occurs after the flag is cleared it must be processed, i.e. this interrupt
         *  exits then immediately starts again.  Most microcontrollers handle this
         *  naturally but it must be verified for every target.
         */
        MRFI_CLEAR_SYNC_PIN_INT_FLAG();

        Mrfi_SyncPinRxIsr();
      }
      else
      {
        /* Fatal error. SYNC interrupt is not supposed to happen at this time. */
        MRFI_FORCE_ASSERT();
      }
    }
    else
    {
      /* Fatal error. No other Radio interurpt is enabled. */
      MRFI_FORCE_ASSERT();
    }
  }
}


/**************************************************************************************************
 * @fn          MRFI_Rssi
 *
 * @brief       Returns "live" RSSI value
 *
 * @param       none
 *
 * @return      RSSI value in units of dBm.
 **************************************************************************************************
 */
int8_t MRFI_Rssi(void)
{
  uint8_t regValue;

  /* Radio must be in RX state to measure rssi. */
  MRFI_ASSERT( mrfiRadioState == MRFI_RADIO_STATE_RX );

  /* Wait for the RSSI to be valid:
   * Just having the Radio ON is not enough to read
   * the correct RSSI value. The Radio must in RX mode for
   * a certain duration. This duration depends on
   * the baud rate and the received signal strength itself.
   */
  MRFI_RSSI_VALID_WAIT();

  /* Read the RSSI value */
  regValue = MRFI_RADIO_REG_READ( RSSI );

  /* convert and do offset compensation */
  return( Mrfi_CalculateRssi(regValue) );
}


/**************************************************************************************************
 * @fn          Mrfi_CalculateRssi
 *
 * @brief       Does binary to decimal conversion and offset compensation.
 *
 * @param       none
 *
 * @return      RSSI value in units of dBm.
 **************************************************************************************************
 */
int8_t Mrfi_CalculateRssi(uint8_t rawValue)
{
  int16_t rssi;

  /* The raw value is in 2's complement and in half db steps. Convert it to
   * decimal taking into account the offset value.
   */
  if(rawValue >= 128)
  {
    rssi = (int16_t)(rawValue - 256)/2 - MRFI_RSSI_OFFSET;
  }
  else
  {
    rssi = (rawValue/2) - MRFI_RSSI_OFFSET;
  }

  /* Restrict this value to least value can be held in an 8 bit signed int */
  if(rssi < -128)
  {
    rssi = -128;
  }

  return rssi;
}

/**************************************************************************************************
 * @fn          MRFI_RandomByte
 *
 * @brief       Returns a random byte. This is a pseudo-random number generator.
 *              The generated sequence will repeat every 256 values.
 *              The sequence itself depends on the initial seed value.
 *
 * @param       none
 *
 * @return      a random byte
 **************************************************************************************************
 */
uint8_t MRFI_RandomByte(void)
{
  mrfiRndSeed = (mrfiRndSeed*MRFI_RANDOM_MULTIPLIER) + MRFI_RANDOM_OFFSET;

  return mrfiRndSeed;
}

/**************************************************************************************************
 * @fn          Mrfi_RandomBackoffDelay
 *
 * @brief       -
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
static void Mrfi_RandomBackoffDelay(void)
{
  uint8_t backoffs;
  uint8_t i;

  /* calculate random value for backoffs - 1 to 16 */
  backoffs = (MRFI_RandomByte() & 0x0F) + 1;

  /* delay for randomly computed number of backoff periods */
  for (i=0; i<backoffs; i++)
  {
    Mrfi_DelayUsec( sBackoffHelper );
  }
}

/****************************************************************************************************
 * @fn          Mrfi_DelayUsec
 *
 * @brief       Execute a delay loop using HW timer. The macro actually used to do the delay
 *              is not thread-safe. This routine makes the delay execution thread-safe by breaking
 *              up the requested delay up into small chunks and executing each chunk as a critical
 *              section. The chunk size is choosen to be the smallest value used by MRFI. The delay
 *              is only approximate because of the overhead computations. It errs on the side of
 *              being too long.
 *
 * input parameters
 * @param   howLong - number of microseconds to delay
 *
 * @return      none
 ****************************************************************************************************
 */
static void Mrfi_DelayUsec(uint16_t howLong)
{
  bspIState_t s;
  uint16_t count = howLong/MRFI_MAX_DELAY_US;

  if (howLong)
  {
    do
    {
      BSP_ENTER_CRITICAL_SECTION(s);
      BSP_DELAY_USECS(MRFI_MAX_DELAY_US);
      BSP_EXIT_CRITICAL_SECTION(s);
    } while (count--);
  }

  return;
}

/****************************************************************************************************
 * @fn          Mrfi_DelayUsecSem
 *
 * @brief       Execute a delay loop using a HW timer. See comments for Mrfi_DelayUsec().
 *              Delay specified number of microseconds checking semaphore for
 *              early-out. Run in a separate thread when the reply delay is
 *              invoked. Cleaner then trying to make MRFI_DelayUsec() thread-safe
 *              and reentrant.
 *
 * input parameters
 * @param   howLong - number of microseconds to delay
 *
 * @return      none
 ****************************************************************************************************
 */
static void Mrfi_DelayUsecSem(uint16_t howLong)
{
  bspIState_t s;
  uint16_t count = howLong/MRFI_MAX_DELAY_US;

  if (howLong)
  {
    do
    {
      BSP_ENTER_CRITICAL_SECTION(s);
      BSP_DELAY_USECS(MRFI_MAX_DELAY_US);
      BSP_EXIT_CRITICAL_SECTION(s);
      if (sKillSem)
      {
        break;
      }
    } while (count--);
  }

  return;
}

/**************************************************************************************************
 * @fn          MRFI_DelayMs
 *
 * @brief       Delay the specified number of milliseconds.
 *
 * @param       milliseconds - delay time
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_DelayMs(uint16_t milliseconds)
{
  while (milliseconds)
  {
    Mrfi_DelayUsec( APP_USEC_VALUE );
    milliseconds--;
  }
}

/**************************************************************************************************
 * @fn          MRFI_ReplyDelay
 *
 * @brief       Delay number of milliseconds scaled by data rate. Check semaphore for
 *              early-out. Run in a separate thread when the reply delay is
 *              invoked. Cleaner then trying to make MRFI_DelayMs() thread-safe
 *              and reentrant.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_ReplyDelay(void)
{
  bspIState_t s;
  uint16_t    milliseconds = sReplyDelayScalar;

  BSP_ENTER_CRITICAL_SECTION(s);
  sReplyDelayContext = 1;
  BSP_EXIT_CRITICAL_SECTION(s);

  while (milliseconds)
  {
    Mrfi_DelayUsecSem( APP_USEC_VALUE );
    if (sKillSem)
    {
      break;
    }
    milliseconds--;
  }

  BSP_ENTER_CRITICAL_SECTION(s);
  sKillSem           = 0;
  sReplyDelayContext = 0;
  BSP_EXIT_CRITICAL_SECTION(s);
}

/**************************************************************************************************
 * @fn          MRFI_PostKillSem
 *
 * @brief       Post to the loop-kill semaphore that will be checked by the iteration loops
 *              that control the delay thread.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_PostKillSem(void)
{

  if (sReplyDelayContext)
  {
    sKillSem = 1;
  }

  return;
}

/**************************************************************************************************
 * @fn          MRFI_GetRadioState
 *
 * @brief       Returns the current radio state.
 *
 * @param       none
 *
 * @return      radio state - off/idle/rx
 **************************************************************************************************
 */
uint8_t MRFI_GetRadioState(void)
{
  return mrfiRadioState;
}


/**************************************************************************************************
 * @fn          MRFI_SetLogicalChannel
 *
 * @brief       Set logical channel.
 *
 * @param       chan - logical channel number
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_SetLogicalChannel(uint8_t chan)
{
  /* logical channel is not valid? */
  MRFI_ASSERT( chan < MRFI_NUM_LOGICAL_CHANS );

  /* make sure radio is off before changing channels */
  Mrfi_RxModeOff();

  MRFI_RADIO_REG_WRITE( CHANNR, mrfiLogicalChanTable[chan] );

  /* turn radio back on if it was on before channel change */
  if(mrfiRadioState == MRFI_RADIO_STATE_RX)
  {
    Mrfi_RxModeOn();
  }
}

/**************************************************************************************************
 * @fn          MRFI_SetRFPwr
 *
 * @brief       Set ouput RF power level.
 *
 * @param       level - power level to be set
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_SetRFPwr(uint8_t level)
{
  /* Power level is not valid? */
  MRFI_ASSERT( level < MRFI_NUM_POWER_SETTINGS );

  MRFI_RADIO_REG_WRITE(PATABLE, mrfiRFPowerTable[level]);

  return;
}
/**************************************************************************************************
 * @fn          MRFI_SetRxAddrFilter
 *
 * @brief       Set the address used for filtering received packets.
 *
 * @param       pAddr - pointer to address to use for filtering
 *
 * @return      zero     : successfully set filter address
 *              non-zero : illegal address
 **************************************************************************************************
 */
uint8_t MRFI_SetRxAddrFilter(uint8_t * pAddr)
{
  /*
   *  If first byte of filter address match fir byte of broadcast address,
   *  there is a conflict with hardware filtering.
   */
  if (pAddr[0] == mrfiBroadcastAddr[0])
  {
    /* unable to set filter address */
    return( 1 );
  }

  /*
   *  Set the hardware address register.  The hardware address filtering only recognizes
   *  a single byte but this does provide at least some automatic hardware filtering.
   */
  MRFI_RADIO_REG_WRITE( ADDR, pAddr[0] );

  /* save a copy of the filter address */
  {
    uint8_t i;

    for (i=0; i<MRFI_ADDR_SIZE; i++)
    {
      mrfiRxFilterAddr[i] = pAddr[i];
    }
  }

  /* successfully set filter address */
  return( 0 );
}


/**************************************************************************************************
 * @fn          MRFI_EnableRxAddrFilter
 *
 * @brief       Enable received packet filtering.
 *
 * @param       none
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_EnableRxAddrFilter(void)
{
  MRFI_ASSERT(mrfiRxFilterAddr[0] != mrfiBroadcastAddr[0]); /* filter address must be set before enabling filter */

  /* set flag to indicate filtering is enabled */
  mrfiRxFilterEnabled = 1;

  /* enable hardware filtering on the radio */
  MRFI_RADIO_REG_WRITE( PKTCTRL1, PKTCTRL1_ADDR_FILTER_ON );
}


/**************************************************************************************************
 * @fn          MRFI_DisableRxAddrFilter
 *
 * @brief       Disable received packet filtering.
 *
 * @param       pAddr - pointer to address to test for filtering
 *
 * @return      none
 **************************************************************************************************
 */
void MRFI_DisableRxAddrFilter(void)
{
  /* clear flag that indicates filtering is enabled */
  mrfiRxFilterEnabled = 0;

  /* disable hardware filtering on the radio */
  MRFI_RADIO_REG_WRITE( PKTCTRL1, PKTCTRL1_ADDR_FILTER_OFF );
}


/**************************************************************************************************
 * @fn          Mrfi_RxAddrIsFiltered
 *
 * @brief       Determine if address is filtered.
 *
 * @param       none
 *
 * @return      zero     : address is not filtered
 *              non-zero : address is filtered
 **************************************************************************************************
 */
uint8_t Mrfi_RxAddrIsFiltered(uint8_t * pAddr)
{
  uint8_t i;
  uint8_t addrByte;
  uint8_t filterAddrMatches;
  uint8_t broadcastAddrMatches;

  /* first check to see if filtering is even enabled */
  if (!mrfiRxFilterEnabled)
  {
    /*
     *  Filtering is not enabled, so by definition the address is
     *  not filtered.  Return zero to indicate address is not filtered.
     */
    return( 0 );
  }

  /* clear address byte match counts */
  filterAddrMatches    = 0;
  broadcastAddrMatches = 0;

  /* loop through address to see if there is a match to filter address of broadcast address */
  for (i=0; i<MRFI_ADDR_SIZE; i++)
  {
    /* get byte from address to check */
    addrByte = pAddr[i];

    /* compare byte to filter address byte */
    if (addrByte == mrfiRxFilterAddr[i])
    {
      filterAddrMatches++;
    }
    if (addrByte == mrfiBroadcastAddr[i])
    {
      broadcastAddrMatches++;
    }
  }

  /*
   *  If address is *not* filtered, either the "filter address match count" or
   *  the "broadcast address match count" will equal the total number of bytes
   *  in the address.
   */
  if ((broadcastAddrMatches == MRFI_ADDR_SIZE) || (filterAddrMatches == MRFI_ADDR_SIZE))
  {
    /* address *not* filtered, return zero */
    return( 0 );
  }
  else
  {
    /* address filtered, return non-zero */
    return( 1 );
  }
}


/**************************************************************************************************
 *                                  Compile Time Integrity Checks
 **************************************************************************************************
 */

/* calculate maximum value for PKTLEN and verify it directly */
#define MRFI_RADIO_TX_FIFO_SIZE     64  /* from datasheet */
#define MRFI_RADIO_MAX_PKTLEN       (MRFI_RADIO_TX_FIFO_SIZE - MRFI_LENGTH_FIELD_SIZE - MRFI_RX_METRICS_SIZE)
#if (MRFI_RADIO_MAX_PKTLEN != 61)
#error "ERROR:  The maximum value for PKTLEN is not correct."
#endif

/* verify setting for PKTLEN does not exceed maximum */
#if (MRFI_SETTING_PKTLEN > MRFI_RADIO_MAX_PKTLEN)
#error "ERROR:  Maximum possible value for PKTLEN exceeded.  Decrease value of maximum payload."
#endif

/* verify largest possible packet fits within FIFO buffer */
#if ((MRFI_MAX_FRAME_SIZE + MRFI_RX_METRICS_SIZE) > MRFI_RADIO_TX_FIFO_SIZE)
#error "ERROR:  Maximum possible packet length exceeds FIFO buffer.  Decrease value of maximum payload."
#endif

/* verify that the SmartRF file supplied is compatible */
#if ((!defined SMARTRF_RADIO_CC430))
  #error "ERROR:  The SmartRF export file is not compatible."
#endif

/*
 *  These asserts happen if there is extraneous compiler padding of arrays.
 *  Modify compiler settings for no padding, or, if that is not possible,
 *  comment out the offending asserts.
 */
BSP_STATIC_ASSERT(sizeof(mrfiRadioCfg) == ((sizeof(mrfiRadioCfg)/sizeof(mrfiRadioCfg[0])) * sizeof(mrfiRadioCfg[0])));


/*
 *  These asserts happen if there is extraneous compiler padding of arrays.
 *  Modify compiler settings for no padding, or, if that is not possible,
 *  comment out the offending asserts.
 */
BSP_STATIC_ASSERT(sizeof(mrfiLogicalChanTable) == ((sizeof(mrfiLogicalChanTable)/sizeof(mrfiLogicalChanTable[0])) * sizeof(mrfiLogicalChanTable[0])));
BSP_STATIC_ASSERT(sizeof(mrfiBroadcastAddr) == ((sizeof(mrfiBroadcastAddr)/sizeof(mrfiBroadcastAddr[0])) * sizeof(mrfiBroadcastAddr[0])));

/**************************************************************************************************
*/
