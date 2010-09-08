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

#ifdef CONFIG_SIDEREAL

// driver
#include "ports.h"
#include "display.h"
#include "timer.h"

#ifdef CONFIG_INFOMEM
#include "infomem.h"
#endif

// logic
#include "menu.h"
#include "clock.h"
#include "sidereal.h"
#include "user.h"


#include "date.h"

#include <string.h>


// *************************************************************************************************
// Prototypes section
void reset_sidereal_clock(void);
void clock_sidereal_tick(void);
void mx_time(u8 line);
void sx_time(u8 line);


// *************************************************************************************************
// Defines section

// for details on used formulas see
// http://www.usno.navy.mil/USNO/astronomical-applications/astronomical-information-center/approx-sider-time

//fixed starting point: 1. Jan 2000 12:00:00 UTC :: sid_seconds=18.697374558*60*60=67310.5484088
//						1. Jan 2000 12:00:00 UTC +165s = 1.1.2000 12:02:45 UTC :: sid_seconds=67476.00016
const u8 fix_sec=45;
const u8 fix_min=2;
const u8 fix_hour=12;
const u8 fix_day=1;
const u8 fix_month=1;
const u16 fix_year=2000;

const unsigned long fix_sidsec=67476;


//rationalisations of 1.002737909350795
const unsigned long rational[][2]={ {440501801,31055},{50828929,78494},{16958560,70591},{1776538,53402},{140253,54237},{46751,46879},{34698,34793},{12053,12086},{1461,1465},{1096,1099},{365,366},{183,184},{100,100},{1,1}};
// rational[n][1] = round(1.002737909350795*rational[n][0]) % 86400
// numbers where chosen to have very small errors from rounding

// rational[n][0]:rational[n][1] ~ fabs(round(1.002737909350795*rational[n][0])-rational[n][1])
//1:1 ~ 0.002737909351
//183:184 ~ 0.4989625888
//365:366 ~ 0.0006630869598
//1096:1099 ~ 0.0007486484715
//1461:1465 ~ 8.55615117e-05
//12053:12086 ~ 2.140513243e-05
//34698:34793 ~ 2.134610986e-05
//46751:46879 ~ 5.902256817e-08
//140253:140637 ~ 1.770677045e-07		1:54237
//1776538:1781402 ~ 2.24285759e-06		20:53402
//16958560:17004991 ~ 1.862645149e-08	196:70591
//50828929:50968094 ~ 0					589:78494
//440501801:441707855 ~ 0				5112:31055


// *************************************************************************************************
// Global Variable section
struct sidereal_time sSidereal_time;


// *************************************************************************************************
// Extern section


// *************************************************************************************************
// @fn          secs_since_fix
// @brief       calculates time difference in seconds since defined fixed point
//              (leap day calculation is simplified and only works for specific fixed points)
// @param       components of time/date
// @return      seconds since fixed time (in solar seconds)
// *************************************************************************************************
unsigned long secs_since_fix(u8 sec, u8 min, u8 hour, u8 day, u8 month, u16 year)
{
	const int days_till_month[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
	
		//assumes fixed point in leap year (years 2000) before feb 29
		short num_leap_years=(year-fix_year)/4;
		if(((year-fix_year)%4>=1) || month>=3 || (month==2 && day==29)) num_leap_years++;
		
		unsigned long result=
		(
			(
				(unsigned long)(
					(short)365*(year-fix_year)
					+(short)(days_till_month[month-1]-days_till_month[fix_month-1])
					+(short)(day-fix_day)
					+num_leap_years //days
				)*24+(long)(hour-fix_hour) //hours
			)*60+(short)(min-fix_min) //minutes
		)*60+(short)sec-fix_sec; //seconds
		
	return result;
};

// *************************************************************************************************
// @fn          sidereal_seconds
// @brief       calculates sidereal second of the day (for Greenwich) (since 00:00:00) from the
//              number of solar seconds since the fixed time.
//              The sidereal seconds of the fixed time are used as a start point
//              deviation from apparent sidereal time (the "real" value) should be less than 2 seconds
// @param       solar seconds difference since fixed point
// @return      sidereal seconds since 00:00:00
// *************************************************************************************************
unsigned long sidereal_seconds(unsigned long rawtime)
{
	unsigned long sidtime=fix_sidsec;
	int currentindex=0;
	while(rawtime>0){
		//use rational approximation of 1.002737909350795 to prevent use of floating point
		//some multiples of 86400 are already lost here
		while(rational[currentindex][0]<=rawtime){
			sidtime+=rational[currentindex][1];
			rawtime-=rational[currentindex][0];
		}
		//skip to next (more inaccurate) rational approximation
		currentindex++;
	}
	//get rid of multiples of days
	return sidtime%86400;
};


// *************************************************************************************************
// @fn          sync_sidereal
// @brief       calculates local sidereal time from solar time and sets sidereal clock
// @param       none
// @return      none
// *************************************************************************************************
void sync_sidereal(void)
{
	unsigned long sidtime=sidereal_seconds(secs_since_fix(sTime.second, sTime.minute, sTime.hour, sDate.day, sDate.month, sDate.year)-360*sTime.UTCoffset);

	//calculate difference of local time from greenwich time
	long localcorr=	(long)(sSidereal_time.lon[sSidereal_time.lon_selection].deg*60
					+ sSidereal_time.lon[sSidereal_time.lon_selection].min)*4
					+ (sSidereal_time.lon[sSidereal_time.lon_selection].sec+7)/15; //round correctly
	//prevent sidtime from becoming negative
	if(localcorr<0 && -localcorr>sidtime)
	{
		sidtime+=86400;
	}
	sidtime+=localcorr;
	//make sure the time is between 00:00:00 and 23:59:59
	if(sidtime>=86400)
	{
		sidtime -=86400;
	}
	
	// Disable interrupts to prevent race conditions
	Timer0_A1_Stop();
	// Set sidereal 24H time to calculated value
	sSidereal_time.hour   = sidtime/3600;
	sidtime %=3600;
	sSidereal_time.minute = sidtime/60;
	sidtime %=60;
	sSidereal_time.second = sidtime;
	// Set clock timer for one sidereal second in the future
	Timer0_A1_Start();
	
	//sync=1: automatically sync only one time
	if (sSidereal_time.sync==1)
	{
		sSidereal_time.sync=0;
	}
	
}

// *************************************************************************************************
// @fn          reset_siderealclock
// @brief       Resets sidereal clock time to 00:00:00, 24H time format.
// @param       none
// @return      none
// *************************************************************************************************
void reset_sidereal_clock(void)
{
	int i;
	
	//Use values for Greenwich
	for(i=0;i<SIDEREAL_NUM_LON;i++)
	{
		sSidereal_time.lon[i].deg=0;
		sSidereal_time.lon[i].min=0;
		sSidereal_time.lon[i].sec=0;
	}
	sTime.UTCoffset=0;
	sSidereal_time.lon_selection=0;
	
	
	#ifdef CONFIG_INFOMEM
	s16 read_size=infomem_app_amount(SIDEREAL_INFOMEM_ID);
	if(read_size>=sizeof(struct longitude)/2 +1)
	{
		u16 buf[SIDEREAL_NUM_LON*sizeof(struct longitude)/2+1];
		read_size=infomem_app_read(SIDEREAL_INFOMEM_ID,buf,SIDEREAL_NUM_LON*sizeof(struct longitude)/2+1,0);
		sTime.UTCoffset=((u8*)buf)[0];
		sSidereal_time.lon_selection=((u8*)buf)[1];
		
		read_size=(read_size-1)*2;
		if(read_size>SIDEREAL_NUM_LON*sizeof(struct longitude))
			read_size=SIDEREAL_NUM_LON*sizeof(struct longitude);
		memcpy(&(sSidereal_time.lon), buf+1, read_size);
	}
	#endif
	
	sSidereal_time.sync=2;
	
	sync_sidereal();

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
//              has three layers of different configuration sets
// @param       u8 line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void mx_sidereal(u8 line)
{
	u8 select;
	s32 hours;
	s32 minutes;
	s32 seconds;
	s32 lon_degrees[SIDEREAL_NUM_LON];
	s32 lon_minutes[SIDEREAL_NUM_LON];
	s32 lon_seconds[SIDEREAL_NUM_LON];
	s32 sync;
	s32 heart;
	u8 level;
	s32 direction[SIDEREAL_NUM_LON];
	s32 UTCoffset;
	u8 * str;
	int i;
	
	// Clear display
	clear_display_all();

	// Convert global time to local variables
	hours		= sSidereal_time.hour;
	minutes 	= sSidereal_time.minute;
	seconds 	= sSidereal_time.second;
	
	sync		= sSidereal_time.sync;
	
	for(i=0; i<SIDEREAL_NUM_LON; i++)
	{
		if(sSidereal_time.lon[i].deg<0 || sSidereal_time.lon[i].min<0 || sSidereal_time.lon[i].sec<0)
		{
			direction[i]=0;
			lon_degrees[i] = -sSidereal_time.lon[i].deg;
			lon_minutes[i] = -sSidereal_time.lon[i].min;
			lon_seconds[i] = -sSidereal_time.lon[i].sec;
		}
		else
		{
			direction[i]=1;
			lon_degrees[i] = sSidereal_time.lon[i].deg;
			lon_minutes[i] = sSidereal_time.lon[i].min;
			lon_seconds[i] = sSidereal_time.lon[i].sec;
		}
	}
	
	UTCoffset = sTime.UTCoffset;

	// Init value index (start with Auto Sync selection)
	select = 0;
	
	heart =0;
	level=sSidereal_time.lon_selection;

	// Loop values until all are set or user breaks	set
	while(1)
	{
		// Idle timeout: exit without saving
		if (sys.flag.idle_timeout)
		{
			display_symbol(LCD_SYMB_AM, SEG_OFF);
			display_symbol(LCD_UNIT_L1_DEGREE, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
			break;
		}
		
		if( heart!=0 )
		{
			if(select<=4)
			{
				//go to longitude settings
				if(heart>0)
				{
					select=5;
					level=0;
				}
				//go to time zone settings
				else
				{
					select=10;
				}
				clear_display_all();
				display_symbol(LCD_SYMB_AM, SEG_OFF);
			}
			else if(select<=9)
			{
				if(heart<0)
				{
					//go to previous level
					if(level>0)
					{
						level-=1;
						select=5;
					}
					//go to time/sync settings
					else
					{
						select=0;
					}
				}
				else
				{
					//go to next level
					if(level<SIDEREAL_NUM_LON-1)
					{
						level+=1;
						select=5;
					}
					//go to time zone settings
					else
					{
						select=10;
					}
				}
				clear_display_all();
				display_symbol(LCD_UNIT_L1_DEGREE, SEG_OFF);
				display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
				display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
			}
			else
			{
				//go to time/sync settings
				if(heart>0)
				{
					select=0;
				}
				//go to longitude settings
				else
				{
					select=5;
					level=SIDEREAL_NUM_LON-1;
				}
				clear_display_all();
				display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
				display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
			}
			heart=0;
		}
			

		// Button STAR (short): save, then exit
		if (button.flag.star)
		{
			//store sync settings
			sSidereal_time.sync=sync;
			
			sTime.UTCoffset = UTCoffset;
			
			for(i=0; i<SIDEREAL_NUM_LON; i++)
			{
				//store direction as sign of longitude
				if(direction[i] & 0x1)
				{
					sSidereal_time.lon[i].deg = lon_degrees[i];
					sSidereal_time.lon[i].min = lon_minutes[i];
					sSidereal_time.lon[i].sec = lon_seconds[i];
				}
				else
				{
					sSidereal_time.lon[i].deg = -lon_degrees[i];
					sSidereal_time.lon[i].min = -lon_minutes[i];
					sSidereal_time.lon[i].sec = -lon_seconds[i];
				}
			}
			//only save new lon_selection when it is clear which level is selected
			if(select<=9 && select>4)
			{
				sSidereal_time.lon_selection=level;
			}
			
			#ifdef CONFIG_INFOMEM
			//store new longitude and time zone in information memory
			u16 buf[SIDEREAL_NUM_LON*sizeof(struct longitude)/2+1];
			((u8*)buf)[0]=sTime.UTCoffset;
			((u8*)buf)[1]=sSidereal_time.lon_selection;
			memcpy(buf+1, &(sSidereal_time.lon),SIDEREAL_NUM_LON*sizeof(struct longitude));
			infomem_app_replace(SIDEREAL_INFOMEM_ID,buf,SIDEREAL_NUM_LON*sizeof(struct longitude)/2+1);
			#endif
			
			//sync time if desired
			if(sync >=1)
			{
				sync_sidereal();
			}
			else
			{
				// Disable interrupts to prevent race conditions
				Timer0_A1_Stop();

				// Store local variables in global sidereal clock time
				sSidereal_time.hour   = hours;
				sSidereal_time.minute = minutes;
				sSidereal_time.second = seconds;

				// Set clock timer for one sidereal second in the future
				Timer0_A1_Start();
			}
			
			// Full display update is done when returning from function
			display_symbol(LCD_SYMB_AM, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
			display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
			display_symbol(LCD_UNIT_L1_DEGREE, SEG_OFF);
			break;
		}

		switch (select)
		{
			case 0: 	// Heart Symbol to switch to longitude settings
				// Display HH:MM (LINE1) and As .SS (LINE2)
				str = itoa(hours, 2, 0);
				display_chars(LCD_SEG_L1_3_2, str, SEG_ON);
				display_symbol(LCD_SEG_L1_COL, SEG_ON);

				str = itoa(minutes, 2, 0);
				display_chars(LCD_SEG_L1_1_0, str, SEG_ON);

				str = itoa(seconds, 2, 0);
				display_chars(LCD_SEG_L2_1_0, str, SEG_ON);
				display_symbol(LCD_SEG_L2_DP, SEG_ON);
							
				str = itoa(sync,1,0);
				display_char(LCD_SEG_L2_3, *str, SEG_ON);
				display_char(LCD_SEG_L2_4, 'A', SEG_ON);
				
				set_value(&heart, 0, 0, -1, 1, SETVALUE_DISPLAY_SYMBOL + SETVALUE_NEXT_VALUE, LCD_ICON_HEART, display_value1);
				select =1;
				break;
			case 1: 	// Set Automatic Sync setings
				set_value(&sync, 1, 0, 0, 2, SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_3, display_value1);
				select =2;
				break;
			case 2:		// Set hours
				set_value(&hours, 2, 0, 0, 23, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_3_2, display_hours_12_or_24);
				select = 3;
				break;

			case 3:		// Set minutes
				set_value(&minutes, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_1_0, display_value1);
				select = 4;
				break;

			case 4:		// Set seconds
				set_value(&seconds, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
				select = 0;
				break;
				
			/*=============================================*/
				
			case 5: 	// Heart Symbol to switch to time settings or UTC offset
				//display current level at free digit next to the degrees
				str = itoa(level+1, 1, 0);
				display_chars(LCD_SEG_L1_0, str, SEG_ON);

				str = itoa(lon_degrees[level], 3, 0);
				display_chars(LCD_SEG_L1_3_1, str, SEG_ON);
				display_symbol(LCD_UNIT_L1_DEGREE, SEG_ON);

				str = itoa(lon_minutes[level], 2, 0);
				display_chars(LCD_SEG_L2_4_3, str, SEG_ON);

				str = itoa(lon_seconds[level], 2, 0);
				display_chars(LCD_SEG_L2_1_0, str, SEG_ON);
				display_symbol(LCD_SEG_L2_COL0, SEG_ON);

				if(direction[level] & 0x1)
				{
					display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
				}
				else
				{
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
				}
				set_value(&heart, 0, 0, -1, 1, SETVALUE_DISPLAY_SYMBOL + SETVALUE_NEXT_VALUE, LCD_ICON_HEART, display_value1);
				select =6;
				break;
				
			case 6:		// Set orientation	
				set_value(direction+level, 0, 0, 0, 1, SETVALUE_ROLLOVER_VALUE +  SETVALUE_NEXT_VALUE + SETVALUE_SWITCH_ARROWS, 0, display_value1);
				select = 7;
				break;

			case 7:		// Set degrees
				set_value(lon_degrees+level, 3, 0, 0, 180, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L1_3_1, display_value1);
				select = 8;
				break;
			case 8:		// Set minutes
				set_value(lon_minutes+level, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_4_3, display_value1);
				select = 9;
				break;

			case 9:		// Set seconds
				set_value(lon_seconds+level, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
				select = 5;
				break;
					
			/*=============================================*/
				
			case 10: 	// Heart Symbol to switch to longitude settings
				if(UTCoffset >= 0)
				{
					str = itoa(UTCoffset, 3, 0);
					if(UTCoffset>0)
					{
						display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
					}
				}
				else
				{
					str = itoa( - UTCoffset, 3, 0);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
				}
				display_chars(LCD_SEG_L1_3_1, str, SEG_ON);
				display_symbol(LCD_SEG_L1_DP1,SEG_ON);
				
				memcpy(str,"UTC",3);
				display_chars(LCD_SEG_L2_4_2, str, SEG_ON);
				
				set_value(&heart, 0, 0, -1, 1, SETVALUE_DISPLAY_SYMBOL + SETVALUE_NEXT_VALUE, LCD_ICON_HEART, display_value1);
				select =11;
				break;
				
			case 11:		// Set UTC OFFSET
				set_value(&UTCoffset, 3, 0, -120, 120, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE + SETVALUE_DISPLAY_ARROWS + SETVALUE_STEP_FIFE, LCD_SEG_L1_3_1, display_value1);
				select = 10;
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

#endif