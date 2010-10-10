/*
 * gps.c
 *
 *  Created on: Oct 9, 2010
 *      Author: gabriel
 */

// *************************************************************************************************
// GPS functions.
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
#include "user.h"
#include "gps.h"

//pfs
#ifndef ELIMINATE_BLUEROBIN
#include "bluerobin.h"
#endif

#ifdef CONFIG_SIDEREAL
#include "sidereal.h"
#endif

#include "date.h"

#ifdef CONFIG_USE_SYNC_TOSET_TIME
#include "rfsimpliciti.h"
#endif

void sx_gps(u8 line);
void mx_gps(u8 line);
void display_gps(u8 line, u8 update);




// *************************************************************************************************
// @fn          sx_gps
// @brief       Direct GPS function
// @param       line		LINE1
// @return      none
// *************************************************************************************************
void sx_gps(u8 line)
{
	// Enable idle timeout
	sys.flag.idle_timeout_enabled = 1;

	while(1)
	    {
		// Idle timeout ---------------------------------------------------------------------
		if (sys.flag.idle_timeout)
			{
				// Clear timeout flag
				sys.flag.idle_timeout = 0;

				// Clear display
				clear_display();
				break;
				// Set display update flags
				//display.flag.full_update = 1;

			}

		if (button.flag.num)
		    {
			  break;
		    }


		if (button.flag.down)
		     {
			  // Clear display
			  clear_display_all();
			  display_chars(LCD_SEG_L1_3_0, (u8*)"CODE", SEG_ON);
			  display_chars(LCD_SEG_L2_4_0, (u8*)"PLEAS", SEG_ON);
			  while(1)
			  {
				  if (sys.flag.idle_timeout)
				  			{
				  				// Clear timeout flag
				  				sys.flag.idle_timeout = 0;

				  				// Clear display
				  				clear_display();
				  				break;
				  				// Set display update flags
				  				//display.flag.full_update = 1;

				  			}
				  idle_loop();
			  }
			  break;
		     }
	    }

		// Clear timeout flag
		sys.flag.idle_timeout = 0;
		// Clear button flags
	    button.all_flags = 0;
	    // Clear display
		clear_display();
		// Force full display update
	    display.flag.full_update = 1;
}

// *************************************************************************************************
// @fn          mx_gps
// @brief       Submenu GPS Function
// @param       u8 line		LINE1, LINE2
// @return      none
// *************************************************************************************************
void mx_gps(u8 line)
{


}

// *************************************************************************************************
// @fn          display_gps
// @brief       Display GPS function
// @param       u8 line			LINE1
//				u8 update		DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_UPDATE_PARTIAL
// @return      none
// *************************************************************************************************
void display_gps(u8 line, u8 update)
{
	if (update == DISPLAY_LINE_UPDATE_FULL)
		{
			display_chars(LCD_SEG_L2_5_0, (u8 *)"   GPS", SEG_ON);
		}
}
