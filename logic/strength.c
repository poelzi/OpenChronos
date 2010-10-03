/*
 * A timer for strength training (particularly for those strength training facilities
 * where exercises are performed for a given amount of time instead of a given
 * number of repetitions, e.g. Kieser Training(TM))
 * 
 * This watch function contributes a menu entry in the first row. 
 * It has three states: initial, running and stopped. In initial state,
 * the display reads 'STRE'. If the user presses UP in initial state,
 * the watch goes into the running state. 
 */
// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"

// logic
#include "menu.h"
#include "strength.h"

// only compile all of this if the user wants it
#ifdef CONFIG_STRENGTH

// ----------------------------------------------------------------------
// Data section

// All the static data
// Fortunately, all-zeros is reset enough.
strength_data_t strength_data = { } ;

// *************************************************************************************************
// @fn          display_strength_time
// @brief       Redisplay the strength time or the strength banner
// @param       line LINE1 or LINE2 (but we pretend to be LINE1 anyway)
// @param       update whether a full update is requested (but we do a full update anyway)
// @return      none
// *************************************************************************************************
void display_strength_time(u8 line, u8 update) 
{
	u8 secs = strength_data.seconds_since_start;
	
	// if there is anything to display, display that
	if(strength_data.flags.running 
	   || strength_data.seconds_since_start != 0) 
	{
		display_chars(LCD_SEG_L1_2_0, strength_data.time, SEG_ON);
	}
	else 
	{
		// yeah, we're the Strength mode.
		display_chars(LCD_SEG_L1_3_0, "STRE", SEG_ON);
	}
	strength_data.flags.redisplay_requested = 0;
}

// *************************************************************************************************
// @fn          strength_tick
// @brief       A second has passed. Recompute strength state. To be called from timer int handler.
// @return      none
// Precondition: the running flag is true. (Just don't call this if Strength is in stopped state)
// *************************************************************************************************
void strength_tick(void)
{
	u8 secs = strength_data.seconds_since_start + 1;
	strength_data.seconds_since_start = secs;
	strength_data.flags.redisplay_requested = 1;

	// simultaneously compute the necessary beeps,
	// the state and the ASCII representation of the current time.
	// dividing is expensive.

	if(secs < STRENGTH_COUNTDOWN_SECS) 
	{
		u8 presecs = STRENGTH_COUNTDOWN_SECS - secs;
		strength_data.time[1] = '-';
		strength_data.time[2] = '0'+presecs;
	} 
	else if (secs == STRENGTH_COUNTDOWN_SECS)
	{
		strength_data.num_beeps = 1;
		strength_data.time[1] = ' ';
		strength_data.time[2] = '0';
	}
	else 
	{
		// beep at important times
		switch(secs) 
		{
		case STRENGTH_THRESHOLD_1: 
			strength_data.num_beeps = 2;
			break;
		case STRENGTH_THRESHOLD_2: 
			strength_data.num_beeps = 3;
			break;
		case STRENGTH_THRESHOLD_END: 
			strength_data.num_beeps = 4;
			strength_data.flags.running = 0;
			break;
		}

		if(++strength_data.time[2] > '9') 
		{
			strength_data.time[2] = '0';
			
                        // space becomes zero, digit stays digit:
			strength_data.time[1] |= '0'; 

			if(++strength_data.time[1] > '9') 
			{
				strength_data.time[0] |= '0'; 
				++strength_data.time[0];
				strength_data.time[1] = '0';
			}
		}
	}

	// strength_data.num_beeps describes the beeping pattern,
	// but since beeping is done in the process_requests phase,
	// we have to set a request flag so that process_requests 
	// is called at all.
	if (strength_data.num_beeps != 0) 
	{
		request.flag.strength_buzzer = 1;
	}
	
}

// *************************************************************************************************
// @fn          strength_reset
// @brief       Reset strength state.
// @return      none
// *************************************************************************************************
void strength_reset(){
	strength_data.num_beeps = 0;
	strength_data.flags.running = 0;
	strength_data.flags.redisplay_requested = 1;
	strength_data.seconds_since_start = 0;
	strength_data.time[0] = ' ';
	strength_data.time[1] = '-';
	strength_data.time[2] = '0' + STRENGTH_COUNTDOWN_SECS;
	strength_data.time[3] = '\0';
}

// *************************************************************************************************
// @fn          strength_sx
// @brief       User has pressed the UP button while in Strength mode; start, stop or reset the time.
// @param       line LINE1 or LINE2 (assumed to be LINE1)
// @return      none
// *************************************************************************************************
void strength_sx(u8 line)
{
	if(strength_data.flags.running) 
	{
		// stop running, but display the result
		strength_data.flags.running = 0;
	}
	else 
	{
		// not running. If a result is being displayed,
		// clear it. Otherwise start running.
		if(strength_data.seconds_since_start != 0) 
		{
			strength_reset();
		}
		else
		{	
			strength_data.flags.running = 1;
		}
	}
	strength_data.flags.redisplay_requested = 1;
}

// *************************************************************************************************
// @fn          strength_display_needs_updating
// @brief       Has the Strength module the desire to redisplay?
// @return      true iff the Strength data has something new to be displayed
// *************************************************************************************************
u8 strength_display_needs_updating(void)
{
	return strength_data.flags.redisplay_requested;
}

#endif // ifdef CONFIG_STRENGTH
