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
// Eggtimer feature
// *************************************************************************************************

// *************************************************************************************************
// Include section

// system
#include "config.h"	//gibbons: bit of a hack: forces next ifdef statement to work
#ifdef CONFIG_EGGTIMER

#include "project.h"

#include <string.h>

// driver
#include "eggtimer.h"
#include "ports.h"
#include "display.h"
#include "timer.h"
#include "buzzer.h"
#include "user.h"

// logic
#include "menu.h"
#include "eggtimer.h"


// *************************************************************************************************
// Prototypes section
void init_eggtimer(void);
void start_eggtimer(void);
void stop_eggtimer(void);
void stop_eggtimer_alarm(void);
void set_eggtimer_to_defaults(void);
void set_eggtimer(void);
void eggtimer_tick(void);
void mx_eggtimer(u8 line);
void sx_eggtimer(u8 line);
void display_eggtimer(u8 line, u8 update);
u8 eggtimer_visible(void);

// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct eggtimer sEggtimer;


// *************************************************************************************************
// Extern section
extern void menu_skip_next(line_t line); // in ezchronos.c


// *************************************************************************************************
// @fn          init_eggtimer
// @brief       Initialize eggtimer; intended to be called once, on firmware restart
// @param       none
// @return      none
// *************************************************************************************************
void init_eggtimer()
{
    sEggtimer.state = EGGTIMER_STOP;
    sEggtimer.duration = EGGTIMER_ALARM_DURATION;
    
    // Set eggtimer default to 1 minute
    sEggtimer.default_hours = 0;
    sEggtimer.default_minutes = 1;
    sEggtimer.default_seconds = 0;
    
    set_eggtimer_to_defaults();
}


// *************************************************************************************************
// @fn          start_eggtimer
// @brief       Sets eggtimer state to on, draws eggtimer icon blinking
// @param       none
// @return      none
// *************************************************************************************************
void start_eggtimer(void)
{
	// Set eggtimer run flag
	sEggtimer.state = EGGTIMER_RUN;

	// Set eggtimer icon (doesn't exist so I wont untill I'll use stopwatch for now)
	display_symbol(LCD_ICON_RECORD, SEG_ON_BLINK_ON);
}


// *************************************************************************************************
// @fn          stop_eggtimer
// @brief       Sets eggtimer state to off, but doesn't reset eggtimer count. 
//		  Also draws eggtimer icon (solid on, no blink)
// @param       none
// @return      none
// *************************************************************************************************
void stop_eggtimer(void)
{	
	// Clear eggtimer run flag
	sEggtimer.state = EGGTIMER_STOP;
	
	// Clear eggtimer icon (doesn't exist so I'll use stopwatch for now)
	display_symbol(LCD_ICON_RECORD, SEG_ON_BLINK_OFF); // Assumes the eggtimer menu is active
}


// *************************************************************************************************
// @fn          stop_eggtimer_alarm
// @brief       Puts eggtimer in STOP mode, halts alarm mode and buzzing if active, updates eggtimer
//			symbol. Safe to call, even if eggtimer menu not active.
// @param       none
// @return      none
// *************************************************************************************************
void stop_eggtimer_alarm(void)
{
	sEggtimer.state = EGGTIMER_STOP;
	sEggtimer.duration = EGGTIMER_ALARM_DURATION;
	if (eggtimer_visible()) {
		display_symbol(LCD_ICON_RECORD, SEG_ON_BLINK_OFF);
	}
	else {
		display_symbol(LCD_ICON_RECORD, SEG_OFF_BLINK_OFF);
	}
	stop_buzzer(); // FIXME: needs to play friendly with other buzzer-using modules (e.g. alarm)
}


// *************************************************************************************************
// @fn          set_eggtimer_to_defaults
// @brief       Clears eggtimer counter; DOES NOT set state to EGGTIMER_STOP! (This way, this
//			function can be called without stopping the alarm.)
// @param       none
// @return      none
// *************************************************************************************************
void set_eggtimer_to_defaults(void)
{
	// Reset eggtimer counts to default (aka last used) values
	sEggtimer.hours = sEggtimer.default_hours;
	sEggtimer.minutes = sEggtimer.default_minutes;
	sEggtimer.seconds = sEggtimer.default_seconds;
	
	if (eggtimer_visible()) {
		display.flag.line2_full_update = 1; // gibbons: this is hardcoded to line 2; change?
	}
}


// *************************************************************************************************
// @fn          set_eggtimer
// @brief       Set's the eggtimer. (Almost entirely copied from the alarm section
// @param       none
// @return      none
// *************************************************************************************************
extern void set_eggtimer(void){
        u8 select;
	s32 hours; // must be s32 to work properly with set_value(...)
	s32 minutes;
        s32 seconds;
        u8 * str;
        
        // Store hours, minutes, and seconds in local variables
        hours   = sEggtimer.hours;
        minutes = sEggtimer.minutes;
        seconds = sEggtimer.seconds;
        
        // Display HH:MM:SS (LINE2)
	str = itoa(hours, 2, 0);
	display_chars(LCD_SEG_L2_5_4, str, SEG_ON);
	str = itoa(minutes, 2, 0);
	display_chars(LCD_SEG_L2_3_2, str, SEG_ON);
	str = itoa(seconds, 2, 0);
	display_chars(LCD_SEG_L2_1_0, str, SEG_ON);

        // Init value index
	select = 0;	
		
	// Loop values until all are set or user breaks set
	while(1) 
	{
		// Idle timeout: exit without saving 
		if (sys.flag.idle_timeout) break;
		
		// M2 (short): save, then exit 
		if (button.flag.num) 
		{
			if ((hours == 0) && (minutes == 0) && (seconds == 0)) { //prevent zero time
				seconds = 1;
			}
			// Store local variables in global Eggtimer default and counters
			sEggtimer.hours = sEggtimer.default_hours = hours;
			sEggtimer.minutes = sEggtimer.default_minutes = minutes;
			sEggtimer.seconds = sEggtimer.default_seconds = seconds;
			break;
		}

		switch (select)
		{
			case 0: // Set hours
			    set_value(&hours, 2, 0, 0, 19, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_5_4, display_value1);
			    select = 1;
			    break;

			case 1:	// Set minutes
			    set_value(&minutes, 2, 0, 0, 99, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_3_2, display_value1);
			    select = 2;
			    break;
                                        
                        case 2:	// Set seconds
			    set_value(&seconds, 2, 0, 0, 99, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
			    select = 0;
			    break;
		}
	}
	
	// Clear button flag
	button.all_flags = 0;
	
}


// *************************************************************************************************
// @fn          eggtimer_tick
// @brief       To be called every second; decreases eggtimer counter and sets display update flag.
// @param       none
// @return      none
// *************************************************************************************************
void eggtimer_tick(void) //gibbons: This function could benefit from an alarm queue...
{
    if (sEggtimer.state != EGGTIMER_RUN) return;
    
    //sEggtimer.drawFlag == 1 --> seconds changed
    //sEggtimer.drawFlag == 2 --> minutes also changed
    //sEggtimer.drawFlag == 3 --> hours also changed
    
    sEggtimer.drawFlag = 1;
    display.flag.update_eggtimer = 1;
    
    // gibbons: Is it possible to merge the if and else if blocks into one?
    if ((sEggtimer.hours == 0) && (sEggtimer.minutes == 0) && (sEggtimer.seconds == 1)) {
	// Die Zeit ist um! Time's up!
	sEggtimer.state = EGGTIMER_ALARM;
	set_eggtimer_to_defaults(); // Set values to defaults, so user can see what time duration just timed out
    }
    else if (sEggtimer.seconds-- == 0) { // NOTE: intentionally sEggtimer.seconds--, and not --sEggtimer.seconds
	sEggtimer.seconds = 59;
	sEggtimer.drawFlag++;
	// Subtract a minute from the remaining time
	if (sEggtimer.minutes-- == 0) {
	    sEggtimer.minutes = 59;
	    sEggtimer.drawFlag++;
	    // Subtract an hour from the remaining time
	    sEggtimer.hours--;
	}
    }
    
}


// *************************************************************************************************
// @fn          mx_eggtimer
// @brief       eggtimer set routine. Mx stops eggtimer and resets count.
// @param       u8 line	LINE2
// @return      none
// *************************************************************************************************
void mx_eggtimer(u8 line)
{
	// Stop eggtimer
	stop_eggtimer();
        
	// Reset eggtimer count to default values
	set_eggtimer_to_defaults();
	
        // Set eggtimer
        set_eggtimer();
			
	// Display eggtimer time
	display_eggtimer(line, DISPLAY_LINE_UPDATE_FULL);
}


// *************************************************************************************************
// @fn          sx_eggtimer
// @brief       eggtimer direct function. S2 starts/stops eggtimer, but does not reset count.
// @param       u8 line	LINE2
// @return      none
// *************************************************************************************************
void sx_eggtimer(u8 line)
{
	if (sEggtimer.state == EGGTIMER_STOP)
	{
		// (Re)start eggtimer
		start_eggtimer();
	}
	else 
	{
		// Stop eggtimer 
		stop_eggtimer();
	}
}


// *************************************************************************************************
// @fn          display_eggtimer
// @brief       eggtimer user routine.
// @param       u8 line		LINE2
//		u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
void display_eggtimer(u8 line, u8 update)
{
	u8 * str;
	
	// Partial line update only
	if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
		// Check draw flag to minimize workload
		switch(sEggtimer.drawFlag) 
		{
		    case 3: // Hours changed
			str = itoa(sEggtimer.hours, 2, 0);
			display_chars(LCD_SEG_L2_5_4, str, SEG_ON);
		    case 2: // Minutes changed
			str = itoa(sEggtimer.minutes, 2, 0);
			display_chars(LCD_SEG_L2_3_2, str, SEG_ON);
		    case 1: // Seconds changed
			str = itoa(sEggtimer.seconds, 2, 0);
			display_chars(LCD_SEG_L2_1_0, str, SEG_ON);
		}
		sEggtimer.drawFlag = 0; // Clear draw flag
	}
	// Redraw whole line
	else if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		// Display HH:MM:SS		
		str = itoa(sEggtimer.hours, 2, 0);
		display_chars(LCD_SEG_L2_5_4, str, SEG_ON);
		str = itoa(sEggtimer.minutes, 2, 0);
		display_chars(LCD_SEG_L2_3_2, str, SEG_ON);
		str = itoa(sEggtimer.seconds, 2, 0);
		display_chars(LCD_SEG_L2_1_0, str, SEG_ON);
		
		display_symbol(LCD_SEG_L2_COL1, SEG_ON);
		display_symbol(LCD_SEG_L2_COL0, SEG_ON);
		
		if (sEggtimer.state != EGGTIMER_STOP) { // Blink if running or alarm triggered
			display_symbol(LCD_ICON_RECORD, SEG_ON_BLINK_ON);
		}
		else { // Solid on if not running
			display_symbol(LCD_ICON_RECORD, SEG_ON_BLINK_OFF);
		}
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		// Stop blinking icon only if eggtimer isn't running
		if (sEggtimer.state == EGGTIMER_STOP) display_symbol(LCD_ICON_RECORD, SEG_OFF);
	}
}


// *************************************************************************************************
// @fn          eggtimer_visible
// @brief       Is eggtimer visible?
// @param       none
// @return      1=Eggtimer menu currently visible, 0=menu not visible
// *************************************************************************************************
u8 eggtimer_visible(void)
{
	return (ptrMenu_L2 == &menu_L2_Eggtimer); // gibbons: currently hardcoded to Line2; change?
}

#endif //ifdef CONFIG_EGGTIMER