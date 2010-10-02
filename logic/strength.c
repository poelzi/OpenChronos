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
	display_chars(LCD_SEG_L1_2_0, (u8*)"WUFF", SEG_ON);
}
/*
 * Pre: running.
 */
void strength_tick(void)
{
	u8 secs = ++ strength_data.seconds_since_start;
	switch(secs) {
	case STRENGTH_COUNTDOWN_SECS: 
		strength_data.num_beeps = 1;
		break;
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
	strength_data.flags.redisplay_requested = 1;
	
}

/* User presses the right-hand button
 */
void strength_sx(u8 line)
{
	if(strength_data.flags.running) 
	{
		strength_data.flags.running = 0;
		// leave junk values in seconds_since_start and num_beeps
	}
	else 
	{
		strength_data.flags.running = 1;
		strength_data.num_beeps = 0;
		strength_data.seconds_since_start = 0;
	}
	strength_data.flags.redisplay_requested = 1;
}

u8 strength_display_needs_updating(void)
{
	return strength_data.flags.redisplay_requested;
}
