// *************************************************************************************************
//
//	Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/ 
//	 
//	 
//	  Redistribution and use in source and binary forms, with or without 
//	  modification, are permitted provided that the following conditions 
//	  are met:
//	
//	    Redistributions of source code must retain the above copyright 
//	    notice, this list of conditions and the following disclaimer.
//	 
//	    Redistributions in binary form must reproduce the above copyright
//	    notice, this list of conditions and the following disclaimer in the 
//	    documentation and/or other materials provided with the   
//	    distribution.
//	 
//	    Neither the name of Texas Instruments Incorporated nor the names of
//	    its contributors may be used to endorse or promote products derived
//	    from this software without specific prior written permission.
//	
//	  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//	  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//	  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//	  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//	  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//	  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//	  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//	  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//	  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//	  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//	  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// *************************************************************************************************
// Radio core access functions. Taken from TI reference code for CC430.
// *************************************************************************************************

// system
#include "project.h"

// driver
#include "rf1a.h"
#include "timer.h"

// logic
#include "rfsimpliciti.h"
//pfs
#ifndef ELIMINATE_BLUEROBIN
#include "bluerobin.h"
#endif


// *************************************************************************************************
// Extern section

// SimpliciTI CC430 radio ISR - located in SimpliciTi library
extern void MRFI_RadioIsr(void);

// BlueRobin CC430 radio ISR - located in BlueRobin library
extern void BlueRobin_RadioISR_v(void);


// *************************************************************************************************
// @fn          radio_reset
// @brief       Reset radio core. 
// @param       none
// @return      none
// *************************************************************************************************
void radio_reset(void)
{
	volatile u16 i;
	u8 x;
	
	// Reset radio core
	Strobe(RF_SRES);
	// Wait before checking IDLE 
	for (i=0; i<100; i++);
	do {
		x = Strobe(RF_SIDLE);
	} while ((x&0x70)!=0x00);  
	
	// Clear radio error register
	RF1AIFERR = 0;
}


// *************************************************************************************************
// @fn          radio_powerdown
// @brief       Put radio to SLEEP mode. 
// @param       none
// @return      none
// *************************************************************************************************
void radio_powerdown(void)
{
  /* Chip bug: Radio does not come out of this SLEEP when put to sleep
   * using the SPWD cmd. However, it does wakes up if SXOFF was used to
   * put it to sleep.
   */
	// Powerdown radio
	Strobe(RF_SIDLE);
	Strobe(RF_SPWD);	
}



// *************************************************************************************************
// @fn          radio_sxoff
// @brief       Put radio to SLEEP mode (XTAL off only). 
// @param       none
// @return      none
// *************************************************************************************************
void radio_sxoff(void)
{
  /* Chip bug: Radio does not come out of this SLEEP when put to sleep
   * using the SPWD cmd. However, it does wakes up if SXOFF was used to
   * put it to sleep.
   */
	// Powerdown radio
	Strobe(RF_SIDLE);
	Strobe(RF_SXOFF);	
}


// *************************************************************************************************
// @fn          open_radio
// @brief       Prepare radio for RF communication. 
// @param       none
// @return      none
// *************************************************************************************************
void open_radio(void)
{
	// Reset radio core
	radio_reset();

	// Enable radio IRQ
	RF1AIFG &= ~BIT4;                         // Clear a pending interrupt
  	RF1AIE  |= BIT4;                          // Enable the interrupt  	
}




// *************************************************************************************************
// @fn          close_radio
// @brief       Shutdown radio for RF communication. 
// @param       none
// @return      none
// *************************************************************************************************
void close_radio(void)
{
	// Disable radio IRQ
	RF1AIFG = 0;
	RF1AIE  = 0; 
		
	// Reset radio core
	radio_reset();
	
	// Put radio to sleep
	radio_powerdown();
}


// *************************************************************************************************
// @fn          GDOx_ISR
// @brief       GDO0/2 ISR to detect received packet. 
//				In BlueRobin mode:  capture packet end time and decode received packet
//				In SimpliciTI mode: go to SimpliciTI handler
// @param       none
// @return      none
// *************************************************************************************************
//pfs 
#ifdef __GNUC__
#include <signal.h>
interrupt (CC1101_VECTOR) radio_ISR(void)
#else
#pragma vector=CC1101_VECTOR
__interrupt void radio_ISR(void)
#endif
{
	u8 rf1aivec = RF1AIV;
	
	// Forward to SimpliciTI interrupt service routine
	if (is_rf())
	{
		MRFI_RadioIsr();
	}
	else // BlueRobin packet end interrupt service routine
	{		
		if (rf1aivec == RF1AIV_RFIFG9)
		{    
		//pfs
		;
		#ifndef ELIMINATE_BLUEROBIN
			if ((sBlueRobin.state == BLUEROBIN_SEARCHING) || (sBlueRobin.state == BLUEROBIN_CONNECTED))
			{
				BlueRobin_RadioISR_v();
			}
		#endif
		}
		else if (rf1aivec == RF1AIV_NONE) // RF1A interface interrupt (error etc.)
		{
			asm("	nop"); // break here
		}
	}
}
