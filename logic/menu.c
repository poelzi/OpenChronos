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
// Menu management functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"

// logic
#include "menu.h"
#include "user.h"
#include "clock.h"
#include "date.h"
#include "alarm.h"
#include "stopwatch.h"
#include "temperature.h"
#include "altitude.h"
#include "battery.h"
//pfs
#ifndef ELIMINATE_BLUEROBIN
#include "bluerobin.h"
#endif
#include "rfsimpliciti.h"
#include "acceleration.h"
#include "rfbsl.h"

#ifdef CONFIG_PHASE_CLOCK
#include "phase_clock.h"
#endif


// *************************************************************************************************
// Defines section
#define FUNCTION(function)  function


// *************************************************************************************************
// Global Variable section
const struct menu * ptrMenu_L1 = NULL;
const struct menu * ptrMenu_L2 = NULL;


// *************************************************************************************************
// Global Variable section

void display_nothing(u8 line, u8 update) {}

u8 update_time(void)
{
	return (display.flag.update_time);
}
u8 update_stopwatch(void)
{
	return (display.flag.update_stopwatch);
}
u8 update_date(void)
{
	return (display.flag.update_date);
}
u8 update_alarm(void)
{
	return (display.flag.update_alarm);
}
u8 update_temperature(void)
{
	return (display.flag.update_temperature);
}
u8 update_battery_voltage(void)
{
	return (display.flag.update_battery_voltage);
}
u8 update_acceleration(void)
{
	return (display.flag.update_acceleration);
}

// *************************************************************************************************
// User navigation ( [____] = default menu item after reset )
//
//	LINE1: 	[Time] -> Alarm -> Temperature -> Altitude -> Heart rate -> Speed -> Acceleration
//
//	LINE2: 	[Date] -> Stopwatch -> Battery  -> ACC -> PPT -> SYNC -> Calories/Distance --> RFBSL
// *************************************************************************************************

// Line1 - Time
const struct menu menu_L1_Time =
{
	FUNCTION(sx_time),			// direct function
	FUNCTION(mx_time),			// sub menu function
	FUNCTION(display_time),		// display function
	FUNCTION(update_time),		// new display data
	&menu_L1_Alarm,
};
// Line1 - Alarm
const struct menu menu_L1_Alarm =
{
	FUNCTION(sx_alarm),			// direct function
	FUNCTION(mx_alarm),			// sub menu function
	FUNCTION(display_alarm),	// display function
	FUNCTION(update_alarm),		// new display data
	&menu_L1_Temperature,
};
// Line1 - Temperature
const struct menu menu_L1_Temperature =
{
	FUNCTION(dummy),					// direct function
	FUNCTION(mx_temperature),			// sub menu function
	FUNCTION(display_temperature),		// display function
	FUNCTION(update_temperature),		// new display data
	&menu_L1_Altitude,
};
// Line1 - Altitude
const struct menu menu_L1_Altitude =
{
	FUNCTION(sx_altitude),				// direct function
	FUNCTION(mx_altitude),				// sub menu function
	FUNCTION(display_altitude),			// display function
	FUNCTION(update_time),				// new display data
	//pfs
	#ifdef ELIMINATE_BLUEROBIN
	&menu_L1_Acceleration,
	#else
	&menu_L1_Heartrate,
	#endif
};

//pfs
#ifndef ELIMINATE_BLUEROBIN
// Line1 - Heart Rate
const struct menu menu_L1_Heartrate =
{
	FUNCTION(sx_bluerobin),				// direct function
	FUNCTION(mx_bluerobin),				// sub menu function
	FUNCTION(display_heartrate),		// display function
	FUNCTION(update_time),				// new display data
	&menu_L1_Speed,
};
// Line1 - Speed
const struct menu menu_L1_Speed =
{
	FUNCTION(dummy),					// direct function
	FUNCTION(dummy),					// sub menu function
	FUNCTION(display_speed),			// display function
	FUNCTION(update_time),				// new display data
	&menu_L1_Acceleration,
};
#endif
// Line1 - Acceleration
const struct menu menu_L1_Acceleration =
{
	FUNCTION(sx_acceleration),			// direct function
	FUNCTION(dummy),					// sub menu function
	FUNCTION(display_acceleration),		// display function
	FUNCTION(update_acceleration),		// new display data
	&menu_L1_Time,
};

// Line2 - Date
const struct menu menu_L2_Date =
{
	FUNCTION(sx_date),			// direct function
	FUNCTION(mx_date),			// sub menu function
	FUNCTION(display_date),		// display function
	FUNCTION(update_date),		// new display data
	&menu_L2_Stopwatch,
};
// Line2 - Stopwatch
const struct menu menu_L2_Stopwatch =
{
	FUNCTION(sx_stopwatch),		// direct function
	FUNCTION(mx_stopwatch),		// sub menu function
	FUNCTION(display_stopwatch),// display function
	FUNCTION(update_stopwatch),	// new display data
	&menu_L2_Battery,
};
// Line2 - Battery 
const struct menu menu_L2_Battery =
{
	FUNCTION(dummy),					// direct function
	FUNCTION(dummy),					// sub menu function
	FUNCTION(display_battery_V),		// display function
	FUNCTION(update_battery_voltage),	// new display data
#ifndef CONFIG_PHASE_CLOCK
	&menu_L2_Rf,
};
#else
	&menu_L2_Phase,
};
// Line2 - ACC (acceleration data + button events via SimpliciTI)
const struct menu menu_L2_Phase =
{
	FUNCTION(sx_phase),				// direct function
	FUNCTION(dummy),				// sub menu function
	FUNCTION(display_phase_clock),	// display function
	FUNCTION(update_time),			// new display data
	&menu_L2_Rf,
};
#endif
// Line2 - ACC (acceleration data + button events via SimpliciTI)
const struct menu menu_L2_Rf =
{
	FUNCTION(sx_rf),				// direct function
	FUNCTION(dummy),				// sub menu function
	FUNCTION(display_rf),			// display function
	FUNCTION(update_time),			// new display data
	&menu_L2_Ppt,
};
// Line2 - PPT (button events via SimpliciTI)
const struct menu menu_L2_Ppt =
{
	FUNCTION(sx_ppt),				// direct function
	FUNCTION(dummy),				// sub menu function
	FUNCTION(display_ppt),			// display function
	FUNCTION(update_time),			// new display data
	&menu_L2_Sync,
};
// Line2 - SXNC (synchronization/data download via SimpliciTI)
const struct menu menu_L2_Sync =
{
	FUNCTION(sx_sync),				// direct function
	FUNCTION(dummy),				// sub menu function
	FUNCTION(display_sync),			// display function
	FUNCTION(update_time),			// new display data
	//pfs
	#ifdef ELIMINATE_BLUEROBIN
	&menu_L2_RFBSL,
	#else
	&menu_L2_CalDist,
	#endif
};
#ifndef ELIMINATE_BLUEROBIN
// Line2 - Calories/Distance
const struct menu menu_L2_CalDist =
{
	FUNCTION(sx_caldist),			// direct function
	FUNCTION(mx_caldist),			// sub menu function
	FUNCTION(display_caldist),		// display function
	FUNCTION(update_time),			// new display data
	&menu_L2_RFBSL,
};
#endif
// Line2 - RFBSL
const struct menu menu_L2_RFBSL =
{
	FUNCTION(sx_rfbsl),				// direct function
	FUNCTION(dummy),				// sub menu function
	FUNCTION(display_rfbsl),		// display function
	FUNCTION(update_time),			// new display data
	&menu_L2_Date,
};
