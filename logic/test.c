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
// Test functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

#ifdef CONFIG_TEST

// driver
#include "display.h"
#ifdef FEATURE_PROVIDE_ACCEL
#include "vti_as.h"
#endif
#include "vti_ps.h"
#include "ports.h"
#include "timer.h"

// logic
#ifdef FEATURE_PROVIDE_ACCEL
#include "acceleration.h"
#endif
#include "altitude.h"
#include "temperature.h"
//pfs
#ifndef ELIMINATE_BLUEROBIN
#include "bluerobin.h"
#endif


// *************************************************************************************************
// Global Variable section

// *************************************************************************************************
// Prototype section
void display_all_on(void);
void display_all_off(void);


// *************************************************************************************************
// @fn          test_mode
// @brief       Manual test mode. Activated by holding buttons STAR and UP simultaneously. 
//				Cancelled by any other button press.
// @param      	none
// @return      none
// *************************************************************************************************
void test_mode(void)
{
	u8 test_step, start_next_test;
	u8 * str;
	u8 i;
	
	// Disable timer - no need for a clock tick
	Timer0_Stop();
	
	// Disable LCD charge pump while in standby mode 
	// This reduces current consumption by ca. 5?A to ca. 10?A
	LCDBVCTL = 0;
	
	// Show welcome screen 
	display_chars(LCD_SEG_L1_3_0, (u8*)"0430", SEG_ON);
	display_chars(LCD_SEG_L2_4_0, (u8*)"CC430", SEG_ON);
	display_symbol(LCD_SEG_L1_COL, SEG_ON);
	display_symbol(LCD_ICON_HEART, SEG_ON);
	display_symbol(LCD_ICON_STOPWATCH, SEG_ON);
	display_symbol(LCD_ICON_RECORD, SEG_ON);
	display_symbol(LCD_ICON_ALARM, SEG_ON);
	display_symbol(LCD_ICON_BEEPER1, SEG_ON);
	display_symbol(LCD_ICON_BEEPER2, SEG_ON);
	display_symbol(LCD_ICON_BEEPER3, SEG_ON);
	display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
	display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
	display_symbol(LCD_SYMB_AM, SEG_ON);

	// Hold watchdog
	WDTCTL = WDTPW + WDTHOLD;

	// Wait for button press 
	_BIS_SR(LPM3_bits + GIE); 
	__no_operation();

	// Clear display
	display_all_off();
	
#ifdef USE_LCD_CHARGE_PUMP
	// Charge pump voltage generated internally, internal bias (V2-V4) generation
	// This ensures that the contrast and LCD control is constant for the whole battery lifetime
	LCDBVCTL = LCDCPEN | VLCD_2_72;
#endif
	
	// Renenable timer
	Timer0_Start();
	
	// Debounce button press
	Timer0_A4_Delay(CONV_MS_TO_TICKS(100));
		
	while(1)
	{
		// Check button event
		if (BUTTON_STAR_IS_PRESSED && BUTTON_UP_IS_PRESSED)
		{
			// Start with test #0
			test_step = 0;
			start_next_test = 1;
			while(1)
			{
				if (start_next_test)
				{
					// Clean up previous test display
					display_all_off();
					
					start_next_test = 0;

					switch (test_step)
					{
						case 0: // All LCD segments on
								display_all_on(); 
								// Wait until buttons are off
								while (BUTTON_STAR_IS_PRESSED && BUTTON_UP_IS_PRESSED);
								break;
						case 1:	// Altitude measurement
#ifdef CONFIG_ALTITUDE
								display_altitude(LINE1, DISPLAY_LINE_UPDATE_FULL);
								for (i=0; i<2; i++)
								{
									while((PS_INT_IN & PS_INT_PIN) == 0); 
									do_altitude_measurement(FILTER_OFF);
									display_altitude(LINE1, DISPLAY_LINE_UPDATE_PARTIAL);
								}
								stop_altitude_measurement();	
#endif
								break;
						case 2: // Temperature measurement
								display_temperature(LINE1, DISPLAY_LINE_UPDATE_FULL);
								for (i=0; i<4; i++)
								{
									Timer0_A4_Delay(CONV_MS_TO_TICKS(250));
									temperature_measurement(FILTER_OFF);
									display_temperature(LINE1, DISPLAY_LINE_UPDATE_PARTIAL);
								}
								break;
						case 3: // Acceleration measurement
#ifdef FEATURE_PROVIDE_ACCEL
								as_start();
								for (i=0; i<4; i++)
								{
									Timer0_A4_Delay(CONV_MS_TO_TICKS(250));
									as_get_data(sAccel.xyz);
									str = itoa( sAccel.xyz[0], 3, 0);
									display_chars(LCD_SEG_L1_2_0, str, SEG_ON);
									str = itoa( sAccel.xyz[2], 3, 0);
									display_chars(LCD_SEG_L2_2_0, str, SEG_ON);
								}
								as_stop();
#endif
								break;
						//pfs
						#ifndef ELIMINATE_BLUEROBIN
						case 4:	// BlueRobin test
								button.flag.up = 1;
								sx_bluerobin(LINE1);
								Timer0_A4_Delay(CONV_MS_TO_TICKS(100));
								get_bluerobin_data();
								display_heartrate(LINE1, DISPLAY_LINE_UPDATE_FULL);
								stop_bluerobin();
								break;
						#endif
					}
					
					// Debounce button
					Timer0_A4_Delay(CONV_MS_TO_TICKS(200));
				}
				
				// Check button event
				if (BUTTON_STAR_IS_PRESSED) 
				{
					test_step = 1;
					start_next_test = 1;
				}
				else if (BUTTON_NUM_IS_PRESSED) 
				{
					test_step = 2;
					start_next_test = 1;
				}
				else if (BUTTON_UP_IS_PRESSED) 
				{
					test_step = 3;
					start_next_test = 1;
				}
				else if (BUTTON_DOWN_IS_PRESSED) 
				{
					test_step = 4;
					start_next_test = 1;
				}
				else if (BUTTON_BACKLIGHT_IS_PRESSED) 
				{
					// Wait until button has been released (avoid restart)
					while (BUTTON_BACKLIGHT_IS_PRESSED);

					// Disable LCD and LCD charge pump
					LCDBCTL0 &= ~BIT0;
					LCDBVCTL  = 0;
					
					// Debounce button press
					Timer0_A4_Delay(CONV_MS_TO_TICKS(500));

					// Disable timer - no need for a clock tick
					Timer0_Stop();
					
					// Hold watchdog
					WDTCTL = WDTPW + WDTHOLD;
					
					// Sleep until button is pressed (ca. 4?A current consumption)
					_BIS_SR(LPM4_bits + GIE);
					__no_operation();
				
					// Force watchdog reset for a clean restart 
					WDTCTL = 1; 
				}
				
#ifdef USE_WATCHDOG		
				// Service watchdog
				WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;
#endif
				// To LPM3
				_BIS_SR(LPM3_bits + GIE);  
				__no_operation();
			}
		}
		else
		{
			// Debounce button
			Timer0_A4_Delay(CONV_MS_TO_TICKS(100));
			button.all_flags = 0;
			break;
		}
	}
}



void display_all_on(void)
{
	u8 * lcdptr = (u8*)0x0A20;
	u8 i;
	
	for (i=1; i<=12; i++) 
	{
		*lcdptr = 0xFF; 
		lcdptr++;
	}
}


#endif // CONFIG_TEST
