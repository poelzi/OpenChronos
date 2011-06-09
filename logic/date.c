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
// Date functions.
// *************************************************************************************************
 

// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"
#include "ports.h"

// logic
#include "date.h"
#include "user.h"
#include "clock.h"

#ifdef CONFIG_SIDEREAL
#include "sidereal.h"
#endif

// *************************************************************************************************
// Prototypes section
void reset_date(void);
u8 get_numberOfDays(u8 month, u16 year);
void add_day(void);
void mx_date(line_t line);
void sx_date(line_t line);
void display_date(line_t line, update_t update);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct date sDate;	


// *************************************************************************************************
// Extern section


// *************************************************************************************************
// @fn          reset_date
// @brief       Reset date to start value.
// @param       none
// @return      none
// *************************************************************************************************
void reset_date(void)
{
	// Set date 
	sDate.year  = 2009;
	sDate.month = 8;
	sDate.day 	= 1;
	
	// Show default display
	sDate.view = 0;

    #if (CONFIG_DST > 0)
    dst_calculate_dates();
    #endif
}


// *************************************************************************************************
// @fn          get_NumberOfDays
// @brief       Return number of days for a given month
// @param       month		month as char
//				year		year as int
// @return      			day count for given month
// *************************************************************************************************
u8 get_numberOfDays(u8 month, u16 year)
{
   switch(month)
   {
      case 1: 	
      case 3: 	
      case 5: 	
      case 7: 	
      case 8: 	
      case 10: 
      case 12: 	return (31);

      case 4: 	
      case 6: 	
      case 9: 	
      case 11: 	return (30);

	  // 1. A year that is divisible by 4 is a leap year. (Y % 4) == 0
	  // 2. Exception to rule 1: a year that is divisible by 100 is not a leap year. (Y % 100) != 0
	  // 3. Exception to rule 2: a year that is divisible by 400 is a leap year. (Y % 400) == 0 
   
      case 2: 	if ((year%4==0) && ((year%100!=0) || (year%400==0)))	
      				return (29);
				else
					return (28);
				
      default: 	return (0);
   }
}


// *************************************************************************************************
// @fn          add_day
// @brief       Add one day to current date. Called when clock changes from 23:59 to 00:00
// @param       none
// @return      none
// *************************************************************************************************
void add_day(void)
{
	// Add 1 day
	sDate.day++;	
	
	// Check if day overflows into next month
	if (sDate.day > get_numberOfDays(sDate.month, sDate.year))
	{
		// Add 1 month and reset to day to 1
		sDate.day = 1;				
		sDate.month++;	
		
		// Check if month overflows into next year
		if (sDate.month > 12)
		{
			// Add 1 year and reset month and day to 1
			sDate.day = 1;				
			sDate.month = 1;	
			sDate.year++;

            #if (CONFIG_DST > 0)
            dst_calculate_dates();
            #endif
		}	
	}
	// Indicate to display function that new value is available
	display.flag.update_date = 1;
}


// *************************************************************************************************
// @fn          mx_date
// @brief       Date set routine.
// @param       line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void mx_date(line_t line)
{
#ifdef CONFIG_USE_SYNC_TOSET_TIME
	return;
#else
	u8 select;
	s32 day;
	s32 month;
	s32 year;
	s16 max_days;
	u8 * str;
			
	// Clear display
	clear_display_all();
			
	// Convert global to local variables
	day 	= sDate.day;
	month 	= sDate.month;
	year 	= sDate.year;
		
	// Init value index
	select = 0;	
	
	// Init display
	// LINE1: YYYY (will be drawn by set_value)
	// LINE2: MM  DD
	
	str = itoa(day, 2, 1);
	display_chars(LCD_SEG_L2_1_0, str, SEG_ON);

	str = itoa(month, 2, 1);
	display_chars(LCD_SEG_L2_5_4, str, SEG_ON);

	// Loop values until all are set or user breaks	set
	while(1) 
	{
		// Idle timeout: exit without saving 
		if (sys.flag.idle_timeout) break;

		// Button STAR (short): save, then exit 
		if (button.flag.star) 
		{
			// Copy local variables to global variables
			sDate.day = day;
			sDate.month = month;
			sDate.year = year;
			#ifdef CONFIG_SIDEREAL
			if(sSidereal_time.sync>0)
				sync_sidereal();
			#endif
            #if (CONFIG_DST > 0)
            dst_calculate_dates();
            #endif
			
			// Full display update is done when returning from function
			break;
		}

		switch (select)
		{
			case 0:		// Set year
						set_value(&year, 4, 0, 2008, 2100, SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_3_0, display_value1);
						select = 1;
						break;
			case 1:		// Set month
						set_value(&month, 2, 1, 1, 12, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_5_4, display_value1);
						select = 2;
						break;
			case 2:		// Set day
						set_value(&day, 2, 1, 1, max_days, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
						select = 0;
						break;
		}
		
		// Check if day is still valid, if not clamp to last day of current month
		max_days = get_numberOfDays(month, year);
		if (day > max_days) day = max_days;
	}
	
	// Clear button flag
	button.all_flags = 0;
#endif
}


// *************************************************************************************************
// @fn          sx_date
// @brief       Date user routine. Toggles view between DD.MM and YYYY.
// @param       line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void sx_date(line_t line)
{
	// Rotate through 4 views
	if (++sDate.view >= 4) sDate.view = 0;
	if(sDate.view ==3) sTime.line2ViewStyle = DISPLAY_DEFAULT_VIEW;
}


// *************************************************************************************************
// @fn          display_date
// @brief       Display date in DD.MM format (metric units) or MM.DD (English units).
// @param       line_t line			LINE1, LINE2
//				update_t update		DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_UPDATE_PARTIAL
// @return      none
// *************************************************************************************************
void display_date(line_t line, update_t update)
{
#ifdef CONFIG_DAY_OF_WEEK
	const u8 weekDayStr[7][3] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
#endif
	u8 * str;
	
	if (update == DISPLAY_LINE_UPDATE_FULL)
	{
		switch (sDate.view)
		{
			case 0: //WWW.DD
				// Convert day to string
#ifdef CONFIG_DAY_OF_WEEK
				str = itoa(sDate.day, 2, 1);
				display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), str, SEG_ON);

				//pfs BEGIN replace year display with day of week
				//pfs algorith from http://klausler.com/new-dayofweek.html
				#define BASE_YEAR 2001 // not a leap year, so no need to add 1
				u8 skew;
				skew = (sDate.year - BASE_YEAR)+(sDate.year - BASE_YEAR)/4; // compute number of leap years since BASE_YEAR
				if ((29 == get_numberOfDays(2, sDate.year)) && (sDate.month < 3))
				  skew--; // if this is a leap year but before February 29
				skew = (skew + sDate.day); // add day of current month
				//add this month's skew value
				switch(sDate.month) {
				  case 5:
					skew += 1;
					break;
				  case 8:
					skew += 2;
					break;
				  case 2:
				  case 3:
				  case 11:
					skew += 3;
					break;
				  case 6:
					skew += 4;
					break;
				  case 9:
				  case 12:
					skew += 5;
					break;
				  case 4:
				  case 7:
					skew += 6;
					break;
				  default:  //January and October
					break;
				}
				skew = skew%7;
				str = (u8 *)weekDayStr[skew];
				display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_4_2), str, SEG_ON);
				display_symbol(switch_seg(line, LCD_SEG_L1_DP1, LCD_SEG_L2_DP), SEG_ON);
				break;
#else
				// skip this view
				sDate.view++;
#endif
			case 1: //MM  DD
				// Convert day to string
				display_symbol(switch_seg(line, LCD_SEG_L1_DP1, LCD_SEG_L2_DP), SEG_ON);
				// display date
#ifndef CONFIG_METRIC_ONLY
				if (!sys.flag.use_metric_units) {
					str = itoa(sDate.day, 2, 0);
					display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), str, SEG_ON);

					// Convert month to string
					str = itoa(sDate.month, 2, 1);
					display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_3_2), str, SEG_ON);
				} else {
#else
				if (1) {
#endif
					str = itoa(sDate.day, 2, 0);
					display_chars(switch_seg(line, LCD_SEG_L1_3_2, LCD_SEG_L2_3_2), str, SEG_ON);
					
					str = itoa(sDate.month, 2, 0);
					display_chars(switch_seg(line, LCD_SEG_L1_1_0, LCD_SEG_L2_1_0), str, SEG_ON);
				}
				break;
			case 2: //YYYY
				// Convert year to string
				str = itoa(sDate.year, 4, 0);
				display_chars(switch_seg(line, LCD_SEG_L1_3_0, LCD_SEG_L2_3_0), str, SEG_ON);
				break;
			default:
				display_time(line, update);
				break;
		}
	}
	else if	(update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
		if ((sDate.view == 3) || (display.flag.update_date))
			display_date(line, DISPLAY_LINE_UPDATE_FULL);
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		// Do some display cleanup (or just do nothing if everything is OK)
		// This is NOT ONLY called on switch to next menu item
	}	
}
