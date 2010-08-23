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
// Sidereal Time functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "ports.h"
#include "display.h"
#include "timer.h"

// logic
#include "menu.h"
#include "clock.h"
#include "sidereal.h"
#include "user.h"


#include "date.h"


// *************************************************************************************************
// Prototypes section
void reset_sidereal_clock(void);
void clock_sidereal_tick(void);
void mx_time(u8 line);
void sx_time(u8 line);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct sidereal_time sSidereal_time;


// *************************************************************************************************
// Extern section

// *************************************************************************************************
// @fn          reset_siderealclock
// @brief       Resets sidereal clock time to 00:00:00, 24H time format.
// @param       none
// @return      none
// *************************************************************************************************
void reset_sidereal_clock(void)
{
	// Disable interrupts to prevent race conditions
	Timer0_A1_Stop();
	// Set main 24H time to start value
	sSidereal_time.hour   = 0;
	sSidereal_time.minute = 0;
	sSidereal_time.second = 0;
	// Set clock timer for one sidereal second in the future
	Timer0_A1_Start();

	// Display style of both lines is default (HH:MM)
	sSidereal_time.line1ViewStyle = DISPLAY_DEFAULT_VIEW;
	sSidereal_time.line2ViewStyle = DISPLAY_DEFAULT_VIEW;

}


// *************************************************************************************************
// @fn          sidereal_clock_tick
// @brief       Add 1 second to display sidereal time
// @param       none
// @return      none
// *************************************************************************************************
void sidereal_clock_tick(void)
{
	// Use sSidereal_time.drawFlag to minimize display updates
	// sSidereal_time.drawFlag = 1: second
	// sSidereal_time.drawFlag = 2: minute, second
	// sSidereal_time.drawFlag = 3: hour, minute
	sSidereal_time.drawFlag = 1;

	// Add 1 second
	sSidereal_time.second++;

	// Add 1 minute
	if (sSidereal_time.second == 60)
	{
		sSidereal_time.second = 0;
		sSidereal_time.minute++;
		sSidereal_time.drawFlag++;

		// Add 1 hour
		if (sSidereal_time.minute == 60)
		{
			sSidereal_time.minute = 0;
			sSidereal_time.hour++;
			sSidereal_time.drawFlag++;

			// Next day
			if (sSidereal_time.hour == 24)
			{
				sSidereal_time.hour = 0;
			}
		}
	}
}


// *************************************************************************************************
// @fn          mx_sidereal
// @brief       Sidereal Clock set routine.
// @param       u8 line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void mx_sidereal(u8 line)
{
	u8 select;
	s32 hours;
	s32 minutes;
	s32 seconds;
	u8 * str;

	// Clear display
	clear_display_all();

	// Convert global time to local variables
	hours		= sSidereal_time.hour;
	minutes 	= sSidereal_time.minute;
	seconds 	= sSidereal_time.second;

	// Init value index
	select = 0;

	// Loop values until all are set or user breaks	set
	while(1)
	{
		// Idle timeout: exit without saving
		if (sys.flag.idle_timeout)
		{
			display_symbol(LCD_SYMB_AM, SEG_OFF);
			break;
		}

		// Button STAR (short): save, then exit
		if (button.flag.star)
		{
			// Disable interrupts to prevent race conditions
			Timer0_A1_Stop();

			// Store local variables in global clock time
			sSidereal_time.hour 	 = hours;
			sSidereal_time.minute = minutes;
			sSidereal_time.second = seconds;

			// Set clock timer for one sidereal second in the future
			Timer0_A1_Start();

			// Full display update is done when returning from function
			display_symbol(LCD_SYMB_AM, SEG_OFF);
			break;
		}

		switch (select)
		{
			case 0:		// Display HH:MM (LINE1) and .SS (LINE2)
				str = itoa(hours, 2, 0);
				display_chars(LCD_SEG_L1_3_2, str, SEG_ON);
				display_symbol(LCD_SEG_L1_COL, SEG_ON);

				str = itoa(minutes, 2, 0);
				display_chars(LCD_SEG_L1_1_0, str, SEG_ON);

				str = itoa(seconds, 2, 0);
				display_chars(LCD_SEG_L2_1_0, str, SEG_ON);
				display_symbol(LCD_SEG_L2_DP, SEG_ON);

				// Set hours
				set_value(&hours, 2, 0, 0, 23, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_3_2, display_hours_12_or_24);
				select = 1;
				break;

			case 1:		// Set minutes
				set_value(&minutes, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_1_0, display_value1);
				select = 2;
				break;

			case 2:		// Set seconds
				set_value(&seconds, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
				select = 0;
				break;
		}
	}

	// Clear button flags
	button.all_flags = 0;
}


// *************************************************************************************************
// @fn          sx_sidereal
// @brief       Sideneal Time user routine. Toggles view style between HH:MM and SS.
// @param       line		LINE1
// @return      none
// *************************************************************************************************
void sx_sidereal(u8 line)
{
	// Toggle display view style
	if (sSidereal_time.line1ViewStyle == DISPLAY_DEFAULT_VIEW) 	sSidereal_time.line1ViewStyle = DISPLAY_ALTERNATIVE_VIEW;
	else 									 					sSidereal_time.line1ViewStyle = DISPLAY_DEFAULT_VIEW;
}

// *************************************************************************************************
// @fn          display_sidereal
// @brief       Sidereal Clock display routine. Supports 24H and 12H time format,
//              through the helper display_hours_with_12_24.
// @param       u8 line			LINE1
//				u8 update		DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_UPDATE_PARTIAL
// @return      none
// *************************************************************************************************
void display_sidereal(u8 line, u8 update)
{
	// Partial update
	if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
		if(sSidereal_time.drawFlag != 0)
		{
			if (sSidereal_time.line1ViewStyle == DISPLAY_DEFAULT_VIEW)
			{
				switch(sSidereal_time.drawFlag)
				{
					case 3:
						display_hours_12_or_24(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_3_2), sSidereal_time.hour, 2, 1, SEG_ON);
					case 2:
						display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), itoa(sSidereal_time.minute, 2, 0), SEG_ON);
				}
			}
			else
			{
				// Seconds are always updated
				display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), itoa(sSidereal_time.second, 2, 0), SEG_ON);
			}
		}
	}
	else if (update == DISPLAY_LINE_UPDATE_FULL)
	{
		if (line == LINE1)
		{
			// display "i" (like in sIdereal) to distinguish sidereal time clock from normal clock
			display_symbol(LCD_UNIT_L1_I, SEG_ON);
		}
		// Full update
		if ( ( line == LINE1 && sSidereal_time.line1ViewStyle == DISPLAY_DEFAULT_VIEW ) || ( line == LINE2 && sSidereal_time.line2ViewStyle == DISPLAY_DEFAULT_VIEW ) )
		{
			// Display hours
			display_hours_12_or_24(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_3_2), sSidereal_time.hour, 2, 1, SEG_ON);
			// Display minute
			display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), itoa(sSidereal_time.minute, 2, 0), SEG_ON);
			display_symbol(switch_seg(line, LCD_SEG_L1_COL, LCD_SEG_L2_COL0), SEG_ON_BLINK_ON);
		}
		else
		{
			// Display seconds
			display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), itoa(sSidereal_time.second, 2, 0), SEG_ON);
			display_symbol(switch_seg(line, LCD_SEG_L1_DP1, LCD_SEG_L2_DP), SEG_ON);
		}
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		display_symbol(switch_seg(line, LCD_SEG_L1_COL, LCD_SEG_L2_COL0), SEG_OFF_BLINK_OFF);
// 		// Change display style to default (HH:MM)
// 		sSidereal_time.line1ViewStyle = DISPLAY_DEFAULT_VIEW;
		// Clean up AM/PM icon
		display_symbol(LCD_SYMB_AM, SEG_OFF);
		// cleanup "i" icon
		display_symbol(LCD_UNIT_L1_I, SEG_OFF);
	}
}

