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
// Wireless Update functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"
#include "ports.h"

// logic
#include "battery.h"
#include "rfbsl.h"
//pfs
#ifndef ELIMINATE_BLUEROBIN
#include "bluerobin.h"
#endif
#include "rfsimpliciti.h"


// *************************************************************************************************
// Global Variable section
u8 locked = 1;


// *************************************************************************************************
// Extern section
extern void menu_skip_next(line_t line); //ezchronos.c


// *************************************************************************************************
// @fn          mx_rfbsl
// @brief       This functions starts the RFBSL
// @param       line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void mx_rfbsl(u8 line)
{
	if (sys.flag.low_battery) return;

    if (locked) {
        message.flag.prepare = 1;
        message.flag.type_locked = 1;
        return;
    }
	
	// Exit if BlueRobin stack is active
	//pfs
	#ifndef ELIMINATE_BLUEROBIN
	if (is_bluerobin()) return;
	#endif
	
	// Exit if SimpliciTI stack is active
	if (is_rf()) return;
	
	// Before entering RFBSL clear the LINE1 Symbols
	display_symbol(LCD_SYMB_AM, SEG_OFF);

	clear_line(LINE1);
	
	// Write RAM to indicate we will be downloading the RAM Updater first
	display_chars(LCD_SEG_L1_3_0, (u8 *)" RAM", SEG_ON);
	
	// Call RFBSL
	CALL_RFSBL();


}


// *************************************************************************************************
// @fn          sx_rfbsl
// @brief       This functions locks/unlocks the RFBSL (or toggles between battery and rfbsl)
// @param       line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void sx_rfbsl(u8 line)
{
#if defined(CONFIG_USE_DISCRET_RFBSL) && defined(CONFIG_BATTERY)
	if (locked) { // Was in battery mode, toggle to rfbsl mode
		locked = 0;

		// The next bit is a little crude, but it works
		clear_line(LINE2);
		display_battery_V(LINE2, DISPLAY_LINE_CLEAR);
		display_rfbsl(LINE2, DISPLAY_LINE_UPDATE_FULL);
	}
	else { // Was in rfbsl mode, toggle to battery mode
		locked = 1;
		clear_line(LINE2);
		display_rfbsl(LINE2, DISPLAY_LINE_CLEAR); // Currently doesn't do anything
		display_battery_V(LINE2, DISPLAY_LINE_UPDATE_FULL);
	}
#else
	message.flag.prepare = 1;
	if(locked) {
		message.flag.type_unlocked = 1;
		locked = 0;
	} else {
		message.flag.type_locked = 1;
		locked = 1;
	}
#endif
}


// *************************************************************************************************
// @fn          nx_rfbsl
// @brief       This function locks the RFBSL and switches to next menu item
// @param       line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void nx_rfbsl(u8 line)
{
	locked = 1;
	menu_skip_next(line);
}


// *************************************************************************************************
// @fn          display_rfbsl
// @brief       RFBSL display routine. 
// @param       u8 line			LINE2
//				u8 update		DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
void display_rfbsl(u8 line, u8 update)
{
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		display_chars(LCD_SEG_L2_5_0, (u8 *)" RFBSL", SEG_ON);
	}
}


// *************************************************************************************************
// @fn          display_discret_rfbsl
// @brief       Discrete RFBSL display routine: auto selects battery or rfbsl, based on current menu
// @param       u8 line			LINE2
//		u8 update		DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
#if defined(CONFIG_USE_DISCRET_RFBSL) && defined(CONFIG_BATTERY)
void display_discret_rfbsl(u8 line, u8 update)
{
	if (locked) { // battery mode
		display_battery_V(line, update);
	}
	else { // rfbsl mode
		display_rfbsl(line, update);
	}
}
#endif
