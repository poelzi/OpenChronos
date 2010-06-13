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
// ADC12 functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include <project.h>

// driver
#include "adc12.h"
#include "timer.h"


// *************************************************************************************************
// Prototypes section


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
u16 adc12_result;
u8  adc12_data_ready;


// *************************************************************************************************
// Extern section


//
//
//// *************************************************************************************************
//// @fn          adc12_single_conversion
//// @brief       Init ADC12. Do single conversion. Turn off ADC12.
//// @param       none
//// @return      none
//// *************************************************************************************************
//u16 adc12_single_conversion(u16 ref_v, u16 adc12_sht, u16 adc12_ssel, u16 adc12_vref, u16 adc12_channel, u16 adc12_conv_delay)
//{
//	u16 adc12_result;
//
//	// Set REF reference voltage to 1.5V (temperature) or 2.5V (battery)	
//	REFCTL0 |= REFMSTR + REFON + ref_v;           
//
//	// Initialize ADC12 
//	ADC12CTL0 = ADC12ON +  adc12_sht;
//	ADC12CTL1 = ADC12SHP + adc12_ssel;        	// Use sampling timer, ADC12CLK = ACLK
//	ADC12CTL2 = ADC12RES_2 + ADC12SR;       	// 12-bit mode, 50ksps
//	ADC12MCTL0 = adc12_vref + adc12_channel; 	// Set reference, set input channel
//
//	// Wait until ADC12 reference voltage has settled
//	Timer0_A4_Delay(CONV_US_TO_TICKS(ADC12_REFERENCE_SETTLING_TIME_USEC));
//
//	// Start conversion
//	ADC12CTL0 |= ADC12ENC | ADC12SC;
//
//	// Wait until ADC12 has finished
//	Timer0_A4_Delay(CONV_US_TO_TICKS(adc12_conv_delay));
//	while ((ADC12CTL1 & ADC12BUSY) == ADC12BUSY);
//	
//	// Store measurement result 
//	adc12_result = ADC12MEM0; 
//	
//	// Shut down ADC12
//	ADC12CTL0 &= ~(ADC12ENC | ADC12SC);
//	ADC12CTL0 &= ~ADC12ON;
//	
//	// Shut down reference voltage 	
//	REFCTL0 &= ~(REFMSTR + REFVSEL_2 + REFON); 
//	
//	// Return ADC result
//	return (adc12_result);
//}


// *************************************************************************************************
// @fn          adc12_single_conversion
// @brief       Init ADC12. Do single conversion. Turn off ADC12.
// @param       none
// @return      none
// *************************************************************************************************
u16 adc12_single_conversion(u16 ref, u16 sht, u16 channel)
{
	// Initialize the shared reference module 
	REFCTL0 |= REFMSTR + ref + REFON;    		// Enable internal reference (1.5V or 2.5V)
  
	// Initialize ADC12_A 
	ADC12CTL0 = sht + ADC12ON;					// Set sample time 
	ADC12CTL1 = ADC12SHP;                     	// Enable sample timer
	ADC12MCTL0 = ADC12SREF_1 + channel;  		// ADC input channel  
	ADC12IE = 0x001;                          	// ADC_IFG upon conv result-ADCMEMO
  
  	// Wait 2 ticks (66us) to allow internal reference to settle
	Timer0_A4_Delay(2);       
	
	// Start ADC12
	ADC12CTL0 |= ADC12ENC;                             		  	

	// Clear data ready flag
  	adc12_data_ready = 0;
  	
  	// Sampling and conversion start  
    ADC12CTL0 |= ADC12SC;                   	
    
    // Wait until ADC12 has finished
    Timer0_A4_Delay(5);       
	while (!adc12_data_ready);
	
	// Shut down ADC12
	ADC12CTL0 &= ~(ADC12ENC | ADC12SC | sht);
	ADC12CTL0 &= ~ADC12ON;
	
	// Shut down reference voltage 	
	REFCTL0 &= ~(REFMSTR + ref + REFON); 
	
	ADC12IE = 0;                          	
	
	// Return ADC result
	return (adc12_result);
}




// *************************************************************************************************
// @fn          ADC12ISR
// @brief       Store ADC12 conversion result. Set flag to indicate data ready.
// @param       none
// @return      none
// *************************************************************************************************
//pfs wrapped the following to accommodate mspgcc compiler
#ifdef __GNUC__
#include <signal.h>
interrupt (ADC12_VECTOR) ADC12ISR (void)
#else
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
#endif
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    		adc12_result = ADC12MEM0;                       // Move results, IFG is cleared
    		adc12_data_ready = 1;
    		_BIC_SR_IRQ(LPM3_bits);   						// Exit active CPU
    		break;
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}


