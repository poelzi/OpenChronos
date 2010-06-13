/**********************************************************************************************
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

// *************************************************************************************************
// Include section
#include "bsp.h"
#include "mrfi.h"
#include "nwk_types.h"
#include "nwk_api.h"
#include "bsp_leds.h"
#include "bsp_buttons.h"
#include "simpliciti.h"


// *************************************************************************************************
// Defines section
#define TIMEOUT					(10u)

// Conversion from msec to ACLK timer ticks
#define CONV_MS_TO_TICKS(msec)         			(((msec) * 32768) / 1000) 

// U16
typedef unsigned short u16;

// *************************************************************************************************
// Prototypes section


// *************************************************************************************************
// Extern section
extern uint8_t sInit_done;

// SimpliciTI has no low power delay function, so we have to use ours
extern void Timer0_A4_Delay(u16 ticks);


// *************************************************************************************************
// Global Variable section
static linkID_t sLinkID1;



// *************************************************************************************************
// @fn          simpliciti_link
// @brief       Init hardware and try to link to access point.
// @param       none
// @return      unsigned char		0 = Could not link, timeout or external cancel.
//									1 = Linked successful.
// *************************************************************************************************
unsigned char simpliciti_link(void)
{
  uint8_t timeout;
  addr_t lAddr;
  uint8_t i;
  uint8_t pwr;
  
  // Configure timer
  BSP_InitBoard();
  
  // Change network address to value set in calling function
  for (i=0; i<NET_ADDR_SIZE; i++)
  {
    lAddr.addr[i] = simpliciti_ed_address[i];
  }
  SMPL_Ioctl(IOCTL_OBJ_ADDR, IOCTL_ACT_SET, &lAddr);
  
  // Set flag	
  simpliciti_flag = SIMPLICITI_STATUS_LINKING;	
	
  /* Keep trying to join (a side effect of successful initialization) until
   * successful. Toggle LEDS to indicate that joining has not occurred.
   */
  timeout = 0;
  while (SMPL_SUCCESS != SMPL_Init(0))
  {
    NWK_DELAY(1000);

    // Service watchdog
	WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;
    
    // Stop connecting after defined numbers of seconds (15)
    if (timeout++ > TIMEOUT) 
    {
		// Clean up SimpliciTI stack to enable restarting
  		sInit_done = 0;
	    simpliciti_flag = SIMPLICITI_STATUS_ERROR;
  		return (0);
    }
    
    // Break when flag bit SIMPLICITI_TRIGGER_STOP is set
    if (getFlag(simpliciti_flag, SIMPLICITI_TRIGGER_STOP)) 
    {
		// Clean up SimpliciTI stack to enable restarting
    	sInit_done = 0;
    	return (0);
	}
  }
  
  // Set output power to +3.3dmB
  pwr = IOCTL_LEVEL_2;
  SMPL_Ioctl(IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SETPWR, &pwr);

  /* Unconditional link to AP which is listening due to successful join. */
  timeout = 0;
  while (SMPL_SUCCESS != SMPL_Link(&sLinkID1))
  {
    NWK_DELAY(1000);
    
    // Service watchdog
	WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;

    // Stop linking after timeout
    if (timeout++ > TIMEOUT) 
    {
		// Clean up SimpliciTI stack to enable restarting
  		sInit_done = 0;
	    simpliciti_flag = SIMPLICITI_STATUS_ERROR;
  		return (0);
    }
    
    // Exit when flag bit SIMPLICITI_TRIGGER_STOP is set
    if (getFlag(simpliciti_flag, SIMPLICITI_TRIGGER_STOP)) 
    {
    	// Clean up SimpliciTI stack to enable restarting
    	sInit_done = 0;
    	return (0); 
	}
  }
  simpliciti_flag = SIMPLICITI_STATUS_LINKED;
  
  return (1);
}



// *************************************************************************************************
// @fn          simpliciti_main_tx_only
// @brief       Get data through callback. Transfer data when external trigger is set.
// @param       none
// @return      none
// *************************************************************************************************
void simpliciti_main_tx_only(void)
{
  while(1)
  {
    // Get end device data from callback function 
    simpliciti_get_ed_data_callback();
    
    // Send data when flag bit SIMPLICITI_TRIGGER_SEND_DATA is set
    if (getFlag(simpliciti_flag, SIMPLICITI_TRIGGER_SEND_DATA)) 
    {
      // Get radio ready. Wakes up in IDLE state.
      SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);
      
      // Acceleration / button events packets are 4 bytes long
      SMPL_SendOpt(sLinkID1, simpliciti_data, 4, SMPL_TXOPTION_NONE);
      
      // Put radio back to SLEEP state
      SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
      
      clearFlag(simpliciti_flag, SIMPLICITI_TRIGGER_SEND_DATA);
    }
    
    // Exit when flag bit SIMPLICITI_TRIGGER_STOP is set
    if (getFlag(simpliciti_flag, SIMPLICITI_TRIGGER_STOP)) 
    {
      	// Clean up SimpliciTI stack to enable restarting
    	sInit_done = 0;
	    break;
	}
  }
}



// *************************************************************************************************
// @fn          simpliciti_main_sync
// @brief       Send ready-to-receive packets in regular intervals. Listen shortly for host reply.
//				Decode received host command and trigger action. 
// @param       none
// @return      none
// *************************************************************************************************
void simpliciti_main_sync(void)
{
	uint8_t len, i;
	uint8_t ed_data[2];
	
	while(1)
	{
		// Sleep 0.5sec between ready-to-receive packets
		// SimpliciTI has no low power delay function, so we have to use ours
		Timer0_A4_Delay(CONV_MS_TO_TICKS(500));
		
		// Get radio ready. Radio wakes up in IDLE state.
      	SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_AWAKE, 0);
		
		// Send 2 byte long ready-to-receive packet to stimulate host reply
		ed_data[0] = SYNC_ED_TYPE_R2R;
		ed_data[1] = 0xCB;
		SMPL_SendOpt(sLinkID1, ed_data, 2, SMPL_TXOPTION_NONE);
		
		// Wait shortly for host reply
		SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_RXON, 0);
		NWK_DELAY(10);
  	
		// Check if a command packet was received
		while (SMPL_Receive(sLinkID1, simpliciti_data, &len) == SMPL_SUCCESS)
		{
 			// Decode received data
			if (len > 0)
			{
				// Use callback function in application to decode data and react
				simpliciti_sync_decode_ap_cmd_callback();
				
				// Get reply data and send out reply packet burst (19 bytes each)
				for (i=0; i<simpliciti_reply_count; i++)
				{
					NWK_DELAY(10);
					simpliciti_sync_get_data_callback(i);
					SMPL_SendOpt(sLinkID1, simpliciti_data, BM_SYNC_DATA_LENGTH, SMPL_TXOPTION_NONE);
				}
			}
  		}

		// Put radio back to sleep  		
  		SMPL_Ioctl( IOCTL_OBJ_RADIO, IOCTL_ACT_RADIO_SLEEP, 0);
  		
  		// Service watchdog
		WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;
  		
		// Exit when flag bit SIMPLICITI_TRIGGER_STOP is set
		if (getFlag(simpliciti_flag, SIMPLICITI_TRIGGER_STOP)) 
		{
			// Clean up SimpliciTI stack to enable restarting
			sInit_done = 0;
			break;
		}
	}
}
