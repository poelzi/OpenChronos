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

#ifdef CONFIG_USE_GPS

#include <string.h>

// driver
#include "ports.h"
#include "display.h"
#include "timer.h"

// logic
#include "menu.h"
#include "clock.h"
#include "user.h"
#include "gps.h"
#include "sequence.h"

//pfs
#ifndef ELIMINATE_BLUEROBIN
#include "bluerobin.h"
#endif

#ifdef CONFIG_SIDEREAL
#include "sidereal.h"
#endif

#include "date.h"


#include "rfsimpliciti.h"
#include "simpliciti.h"


void sx_gps(u8 line);
void mx_gps(u8 line);
void display_gps(u8 line, u8 update);


void doorlock_signal_success();
void doorlock_signal_failure();
void doorlock_signal_timeout();
void doorlock_signal_invalid();

u8 verify_code();

u8 sequence_saved[DOORLOCK_SEQUENCE_MAX_LENGTH] = {0};
u8 sequence[DOORLOCK_SEQUENCE_MAX_LENGTH] = {0};

// *************************************************************************************************
// @fn          sx_gps
// @brief       Direct GPS function
// @param       line		LINE1
// @return      none
// *************************************************************************************************
void sx_gps(u8 line)
{

	u8 sequence_again[DOORLOCK_SEQUENCE_MAX_LENGTH] = {0};
	u8 error = DOORLOCK_ERROR_SUCCESS;
	u8 i = 0;
	u16 avg = 0;

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

			error = verify_code();
			if (error==DOORLOCK_ERROR_SUCCESS){
				if (sys.flag.low_battery) break;
				 // display_sync(LINE2, DISPLAY_LINE_UPDATE_FULL);
				clear_display_all();
				display_chars(LCD_SEG_L1_3_0, (u8*)"LINK", SEG_ON_BLINK_ON);
				//display_chars(LCD_SEG_L2_4_0, (u8*)"   ", SEG_ON);
				  start_simpliciti_sync();
				 display_chars(LCD_SEG_L1_3_0, (u8*)"LINK", SEG_ON_BLINK_OFF);
				  if(simpliciti_flag == SIMPLICITI_STATUS_ERROR);
				  {
					  display_chars(LCD_SEG_L1_3_0, (u8*)"OUT ", SEG_ON);
					  display_chars(LCD_SEG_L2_4_0, (u8*)"RANGE", SEG_ON);
					  doorlock_signal_timeout();
				  }
			}
			break;
			}

		idle_loop();
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
	u8 error = DOORLOCK_ERROR_SUCCESS;

	if (sequence_saved[0] != 0)
				{
					// Clear display
					 clear_display_all();

					 display_chars(LCD_SEG_L1_3_0, (u8*)" OLD", SEG_ON);
					 display_chars(LCD_SEG_L2_4_0, (u8*)"CODE", SEG_ON);

					 error = verify_code();
					 if (error != DOORLOCK_ERROR_SUCCESS ) return;
				}
					// Clear display
					clear_display_all();

					 display_chars(LCD_SEG_L1_3_0, (u8*)" NEW", SEG_ON);
					 display_chars(LCD_SEG_L2_4_0, (u8*)"CODE", SEG_ON);
					sequence_saved[0] = 0;
					error = verify_code();
					if (error == DOORLOCK_ERROR_SUCCESS ) {
						memcpy(sequence_saved,sequence,DOORLOCK_SEQUENCE_MAX_LENGTH);


						display_chars(LCD_SEG_L1_3_0, (u8*)"CODE", SEG_ON);
						display_chars(LCD_SEG_L2_4_0, (u8*)"AGAIN", SEG_ON);
						error = verify_code();
						if (error == DOORLOCK_ERROR_SUCCESS ) memcpy(sequence_saved,sequence,DOORLOCK_SEQUENCE_MAX_LENGTH);
						else {
							display_chars(LCD_SEG_L1_3_0, (u8*)"CODE", SEG_ON);
							display_chars(LCD_SEG_L2_4_0, (u8*)"FAIL", SEG_ON);
							doorlock_signal_failure();
							sequence_saved[0] = 0;
							}
					}

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



// *************************************************************************************************
// Simple user notification via buzzer
// *************************************************************************************************

// *************************************************************************************************
// @fn          doorlock_signal_success
// @brief       simple two beeps means success
// @param       none
// @return      none
// *************************************************************************************************
void doorlock_signal_success()
{
	start_buzzer(2, CONV_MS_TO_TICKS(100), CONV_MS_TO_TICKS(50));
    Timer0_A4_Delay(CONV_MS_TO_TICKS(300));
    stop_buzzer();
}

// *************************************************************************************************
// @fn          doorlock_signal_failure
// @brief       3 beeps means failure
// @param       none
// @return      none
// *************************************************************************************************
void doorlock_signal_failure()
{
	start_buzzer(3, CONV_MS_TO_TICKS(100), CONV_MS_TO_TICKS(50));
    //Timer0_A4_Delay(CONV_MS_TO_TICKS(450));
	Timer0_A4_Delay(CONV_MS_TO_TICKS(700));
    stop_buzzer();
}

// *************************************************************************************************
// @fn          doorlock_signal_timeout
// @brief       4 beeps means timeout
// @param       none
// @return      none
// *************************************************************************************************
void doorlock_signal_timeout()
{
	start_buzzer(4, CONV_MS_TO_TICKS(100), CONV_MS_TO_TICKS(50));
    Timer0_A4_Delay(CONV_MS_TO_TICKS(600));
    stop_buzzer();
}

// *************************************************************************************************
// @fn          doorlock_signal_invalid
// @brief       5 beeps means something terrible has happened, e.g. someone is trying to hack us
// @param       none
// @return      none
// *************************************************************************************************
void doorlock_signal_invalid()
{
	start_buzzer(1, CONV_MS_TO_TICKS(1000), CONV_MS_TO_TICKS(10));
    Timer0_A4_Delay(CONV_MS_TO_TICKS(1010));
    stop_buzzer();
}

u8 verify_code()
{
	u8 error=DOORLOCK_ERROR_FAILURE;


		  error = doorlock_sequence(sequence);


		  if (error == DOORLOCK_ERROR_SUCCESS)
			{
				//display_chars(LCD_SEG_L1_3_0, (u8*)"CODE", SEG_ON);
				//display_chars(LCD_SEG_L2_4_0, (u8*)"  OK", SEG_ON);
				//doorlock_signal_success();

				if (sequence_saved[0] != 0){
					error = sequence_compare(sequence_saved,sequence);
				}
				else {
					error = DOORLOCK_ERROR_SUCCESS;
				}

				if (error == DOORLOCK_ERROR_SUCCESS){
					display_chars(LCD_SEG_L1_3_0, (u8*)"CODE", SEG_ON);
					display_chars(LCD_SEG_L2_4_0, (u8*)"CHECK", SEG_ON);
					doorlock_signal_success();
					return DOORLOCK_ERROR_SUCCESS;
				}
				else{
					 display_chars(LCD_SEG_L1_3_0, (u8*)"MIS-", SEG_ON);
					 display_chars(LCD_SEG_L2_4_0, (u8*)"MATCH", SEG_ON);
					 doorlock_signal_failure();
				}

			   // memcpy(sequence_saved,sequence,DOORLOCK_SEQUENCE_MAX_LENGTH);
			}
			else
			{
				  display_chars(LCD_SEG_L1_3_0, (u8*)"CODE", SEG_ON);
				  display_chars(LCD_SEG_L2_4_0, (u8*)"FAIL", SEG_ON);
				  doorlock_signal_failure();
			}
	return DOORLOCK_ERROR_FAILURE;

}
#endif // CONFIG_USE_GPS