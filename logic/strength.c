/* TODO add copyright header */
// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"

// logic
#include "menu.h"
#include "strength.h"

strength_data_t strength_data = { } ;

/* TODO add display function */
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
/*
 * Pre: running.
 */
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

	
}


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

/* User presses the right-hand button
 */
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

u8 strength_display_needs_updating(void)
{
	return strength_data.flags.redisplay_requested;
}
