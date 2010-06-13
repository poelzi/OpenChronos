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
// Alarm routines.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"
#include "buzzer.h"
#include "ports.h"

// logic
#include "alarm.h"
#include "clock.h"
#include "user.h"


// *************************************************************************************************
// Prototypes section


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct alarm sAlarm;


// *************************************************************************************************
// Extern section


// *************************************************************************************************
// @fn          clearalarmData
// @brief       Resets alarmData to 06:30
// @param       none
// @return      none
// *************************************************************************************************
void reset_alarm(void) 
{
	// Default alarm time 06:30
	sAlarm.hour   = 06;
	sAlarm.minute = 30;

	// Alarm is initially off	
	sAlarm.duration = ALARM_ON_DURATION;
	sAlarm.state 	= ALARM_DISABLED;
}


// *************************************************************************************************
// @fn          check_alarm
// @brief       Check if current time matches alarm time
// @param       none
// @return      none
// *************************************************************************************************
void check_alarm(void) 
{
	// Return if alarm is not enabled
	if (sAlarm.state != ALARM_ENABLED) return;
	
	// Compare current time and alarm time
	// Start with minutes - only 1/60 probability to match
	if (sTime.minute == sAlarm.minute)
	{
		if (sTime.hour == sAlarm.hour)
		{
			// Indicate that alarm is on
			sAlarm.state = ALARM_ON;
		}
	}
}	


// *************************************************************************************************
// @fn          stop_alarm
// @brief       Stop active alarm
// @param       none
// @return      none
// *************************************************************************************************
void stop_alarm(void) 
{
	// Indicate that alarm is enabled, but not active
	sAlarm.state = ALARM_ENABLED;
	
	// Stop buzzer
	stop_buzzer();
}	


// *************************************************************************************************
// @fn          sx_alarm
// @brief       Sx button turns alarm on/off.
// @param       u8 line		LINE1
// @return      none
// *************************************************************************************************
void sx_alarm(u8 line)
{
	// UP: Alarm on, off
	if(button.flag.up)
	{
		// Toggle alarm state
		if (sAlarm.state == ALARM_DISABLED)		
		{
			sAlarm.state = ALARM_ENABLED;
			
			// Show "  on" message 
			message.flag.prepare = 1;
			message.flag.type_alarm_on = 1;
		}
		else if (sAlarm.state == ALARM_ENABLED)	
		{
			sAlarm.state = ALARM_DISABLED;

			// Show "  off" message 
			message.flag.prepare = 1;
			message.flag.type_alarm_off = 1;
		}
	}
}


// *************************************************************************************************
// @fn          mx_alarm
// @brief       Set alarm time.
// @param       u8 line		LINE1
// @return      none
// *************************************************************************************************
void mx_alarm(u8 line)
{
	u8 select;
	s32 hours;
	s32 minutes;
	u8 * str;
	
	// Clear display
	clear_display_all();

	// Keep global values in case new values are discarded
	hours 		= sAlarm.hour;
	minutes 	= sAlarm.minute;

	// Display HH:MM (LINE1) 
	str = itoa(hours, 2, 0);
	display_chars(LCD_SEG_L1_3_2, str, SEG_ON);
	display_symbol(LCD_SEG_L1_COL, SEG_ON);
	
	str = itoa(minutes, 2, 0);
	display_chars(LCD_SEG_L1_1_0, str, SEG_ON);
	
	// Display "ALARM" (LINE2)
//	display_chars(LCD_SEG_L2_4_0, (u8 *)"ALARM", SEG_ON);
		
	// Init value index
	select = 0;	
		
	// Loop values until all are set or user breaks	set
	while(1) 
	{
		// Idle timeout: exit without saving 
		if (sys.flag.idle_timeout) break;
		
		// STAR (short): save, then exit 
		if (button.flag.star) 
		{
			// Store local variables in global alarm time
			sAlarm.hour = hours;
			sAlarm.minute = minutes;
			// Set display update flag
			display.flag.line1_full_update = 1;
			break;
		}

		switch (select)
		{
			case 0:		// Set hour
                                        set_value(&hours, 2, 0, 0, 23, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_3_2, display_hours1);
                                        select = 1;
                                        break;

			case 1:		// Set minutes
                                        set_value(&minutes, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_1_0, display_value1);
                                        select = 0;
                                        break;
		}
	}
	
	// Clear button flag
	button.all_flags = 0;
	
	// Indicate to display function that new value is available
	display.flag.update_alarm = 1;
}


// *************************************************************************************************
// @fn          display_alarm
// @brief       Display alarm time. 24H / 12H time format.
// @param       u8 line	LINE1, LINE2
//		u8 update	DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_alarm(u8 line, u8 update)
{
	u8 hour12;
	
	if (update == DISPLAY_LINE_UPDATE_FULL)			
	{
		if (sys.flag.use_metric_units)
		{
			// Display 24H alarm time "HH:MM"
			display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_3_2), itoa(sAlarm.hour, 2, 0), SEG_ON); 
		}
		else
		{
			// Display 12H alarm time "HH:MM" + AM/PM
			hour12 = convert_hour_to_12H_format(sAlarm.hour);
			display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_3_2), itoa(hour12, 2, 0), SEG_ON); 	
			
			// Display AM/PM symbol
			display_am_pm_symbol(sAlarm.hour);		
		}
		display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), itoa(sAlarm.minute, 2, 0), SEG_ON); 
		display_symbol(switch_seg(line, LCD_SEG_L1_COL, LCD_SEG_L2_COL0), SEG_ON);

		// Show blinking alarm icon
		display_symbol(LCD_ICON_ALARM, SEG_ON_BLINK_ON);

//		// If alarm is enabled, show icon
// 		if (sAlarm.state == ALARM_ENABLED)
// 		{
// 			display_symbol(LCD_ICON_ALARM, SEG_ON_BLINK_OFF);
// 		}
//		// When alarm is disabled, blink icon to indicate that this is not current time!
// 		else if (sAlarm.state == ALARM_DISABLED) 
// 		{
// 		}
	}
	else if (update == DISPLAY_LINE_CLEAR)			
	{
		// Clean up function-specific segments before leaving function
		display_symbol(LCD_SYMB_AM, SEG_OFF);
		
		// Clear / set alarm icon
		if (sAlarm.state == ALARM_DISABLED) 
		{
			display_symbol(LCD_ICON_ALARM, SEG_OFF_BLINK_OFF);
		}
		else
		{
			display_symbol(LCD_ICON_ALARM, SEG_ON_BLINK_OFF);
		}
	}
}
