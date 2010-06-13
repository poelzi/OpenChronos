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
// Battery voltage measurement functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"
#include "ports.h"
#include "adc12.h"

// logic
#include "menu.h"
#include "battery.h"


// *************************************************************************************************
// Prototypes section
void reset_batt_measurement(void);
void battery_measurement(void);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct batt sBatt;


// *************************************************************************************************
// Extern section
extern void (*fptr_lcd_function_line2)(u8 line, u8 update);


// *************************************************************************************************
// @fn          reset_temp_measurement
// @brief       Reset temperature measurement module.
// @param       none
// @return      none
// *************************************************************************************************
void reset_batt_measurement(void)
{
	// Set flag to off
  	sBatt.state = MENU_ITEM_NOT_VISIBLE; 
	
	// Reset lobatt display counter
	sBatt.lobatt_display = BATTERY_LOW_MESSAGE_CYCLE;
	
	// Start with battery voltage of 3.00V 
	sBatt.voltage = 300;
}


// *************************************************************************************************
// @fn          battery_measurement
// @brief       Init ADC12. Do single conversion of AVCC voltage. Turn off ADC12.
// @param       none
// @return      none
// *************************************************************************************************
void battery_measurement(void)
{
	u16 voltage;
	
	// Convert external battery voltage (ADC12INCH_11=AVCC-AVSS/2)
	//voltage = adc12_single_conversion(REFVSEL_2, ADC12SHT0_10, ADC12SSEL_0, ADC12SREF_1, ADC12INCH_11, ADC12_BATT_CONVERSION_TIME_USEC);
	voltage = adc12_single_conversion(REFVSEL_1, ADC12SHT0_10, ADC12INCH_11);

	// Convert ADC value to "x.xx V"
	// Ideally we have A11=0->AVCC=0V ... A11=4095(2^12-1)->AVCC=4V
	// --> (A11/4095)*4V=AVCC --> AVCC=(A11*4)/4095
	voltage = (voltage * 2 * 2) / 41;  

	// Correct measured voltage with calibration value
	voltage += sBatt.offset;
	
	// Discard values that are clearly outside the measurement range 
	if (voltage > BATTERY_HIGH_THRESHOLD) 
	{
		voltage = sBatt.voltage;
	}
	
	// Filter battery voltage
	sBatt.voltage = ((voltage*2) + (sBatt.voltage*8))/10;

	// If battery voltage falls below low battery threshold, set system flag and modify LINE2 display function pointer
	if (sBatt.voltage < BATTERY_LOW_THRESHOLD) 
	{
		sys.flag.low_battery = 1;
		
		// Set sticky battery icon
		display_symbol(LCD_SYMB_BATTERY, SEG_ON);
	}
	else
	{
		sys.flag.low_battery = 0;

		// Clear sticky battery icon
		display_symbol(LCD_SYMB_BATTERY, SEG_OFF);
	}
	// Update LINE2
	display.flag.line2_full_update = 1;
	
	// Indicate to display function that new value is available
	display.flag.update_battery_voltage = 1;
}




// *************************************************************************************************
// @fn          display_battery_V
// @brief       Display routine for battery voltage. 
// @param       u8 line		LINE2
//				u8 update		DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_battery_V(u8 line, u8 update)
{
	u8 * str;
	
	// Redraw line
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		// Set battery and V icon
		display_symbol(LCD_SYMB_BATTERY, SEG_ON);

		// Menu item is visible
		sBatt.state = MENU_ITEM_VISIBLE; 
		
		// Display result in xx.x format
		str = itoa(sBatt.voltage, 3, 0);

		display_chars(LCD_SEG_L2_2_0, str, SEG_ON);
		display_symbol(LCD_SEG_L2_DP, SEG_ON);
	}
	else if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
		// Display result in xx.x format
		str = itoa(sBatt.voltage, 3, 0);

		display_chars(LCD_SEG_L2_2_0, str, SEG_ON);
			
		display.flag.update_battery_voltage = 0;
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		// Menu item is not visible
		sBatt.state = MENU_ITEM_NOT_VISIBLE; 		
		
		// Clear function-specific symbols
		display_symbol(LCD_SYMB_BATTERY, SEG_OFF);
	}
}



