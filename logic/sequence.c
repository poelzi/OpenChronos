/*
 * sequence.c
 *
 *  Created on: Oct 10, 2010
 *      Author: gabriel
 */


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
#include "vti_as.h"
#include "buzzer.h"
// logic
#include "sequence.h"
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
/*
#ifdef CONFIG_USE_SYNC_TOSET_TIME
#include "rfsimpliciti.h"
#endif
*/
// *************************************************************************************************
// Prototypes section

extern void idle_loop(void);

u8 doorlock_sequence(u8 sequence[DOORLOCK_SEQUENCE_MAX_LENGTH]);
void doorlock_sequence_timer(void);
void doorlock_sequence_pause_timer(void);
u8 sequence_compare(u8* sequence_a, u8* sequence_b);


// *************************************************************************************************
// Global variable section

volatile u8 doorlock_sequence_pause = 0;
volatile u8 doorlock_sequence_timeout = 0;

// *************************************************************************************************
// @fn          doorlock_sequence
// @brief       collects door unlock code sequence using accelerometer
// @param       normalized code sequence (output)
// @return      doorlock error code
// *************************************************************************************************
u8 doorlock_sequence(u8 sequence[DOORLOCK_SEQUENCE_MAX_LENGTH])
{
	s16 previous_delta = 0;
	s16 delta = 0, ddelta = 0;
	u8 previous_raw = 0;
	u8 length = 0;
	u8 max = 0;
	u8 raw = 0;
	u8 i = 0;
	float ratio = 0.0f;

	// initialize
	memset(sequence, 0, sizeof(u8) * DOORLOCK_SEQUENCE_MAX_LENGTH);
	doorlock_sequence_pause = 0;

	// setup timeout
	doorlock_sequence_timeout = DOORLOCK_SEQUENCE_TIMEOUT;

	// start acceleration measurement
	//as_start(AS_MODE_2G_400HZ);
	as_start();

	fptr_Timer0_A1_function = doorlock_sequence_timer;
	Timer0_A1_Start(32768u);
	//Timer0_A1_Start();


	for(;;)
	{

		    // Reset IRQ flags
		BUTTONS_IFG &= ~ALL_BUTTONS;

			// Enable button interrupts
		BUTTONS_IE &= ~ALL_BUTTONS;

		idle_loop();



		if (!doorlock_sequence_timeout)
		{
			as_stop();
			// Reset IRQ flags
				BUTTONS_IFG &= ~ALL_BUTTONS;

				// Enable button interrupts
				BUTTONS_IE |= ALL_BUTTONS;
			return DOORLOCK_ERROR_TIMEOUT;
		}

		// were we interrupted because pause is too long?
		if (doorlock_sequence_pause <= DOORLOCK_SEQUENCE_PAUSE_MAX_LENGTH)
		{
			// look for accelerometer data ready event
			if (!request.flag.acceleration_measurement)
			{
				continue;
			}

			request.flag.acceleration_measurement = 0;

			// read accelerometer z-a
			raw = as_get_z();
			delta = raw - previous_raw;
			ddelta = delta - previous_delta;
			previous_raw = raw;
			previous_delta = delta;

			// proceed if the acceleration is big enough
			if (ddelta < DOORLOCK_SEQUENCE_TAP_THRESHOLD &&
				ddelta > -DOORLOCK_SEQUENCE_TAP_THRESHOLD)
			{
				continue;
			}

			Timer0_A1_Stop();

			// first tap?
			if (length == 0)
			{
				// reset timer
				doorlock_sequence_timeout = 1; // no more timeout

				// reset pause length
				doorlock_sequence_pause = 0;
				++ length;

				// successfully detected a knock, beep once to signal that
				display_symbol(LCD_ICON_RECORD, SEG_ON);
				start_buzzer(1, CONV_MS_TO_TICKS(20), CONV_MS_TO_TICKS(10));
				Timer0_A4_Delay(CONV_MS_TO_TICKS(30));
				stop_buzzer();
				display_symbol(LCD_ICON_RECORD, SEG_OFF);

				// start pause timer
				fptr_Timer0_A1_function = doorlock_sequence_pause_timer;
				Timer0_A1_Start(DOORLOCK_SEQUENCE_PAUSE_RESOLUTION);
				continue;
			}

			// is pause long enough to qualify?
			if (doorlock_sequence_pause > DOORLOCK_SEQUENCE_PAUSE_MIN_LENGTH)
			{
				sequence[length - 1] = doorlock_sequence_pause;
				++ length;
				if (doorlock_sequence_pause > max)
				{
					// also get the biggest pause
					max = doorlock_sequence_pause;
				}

				// successfully detected a knock, beep once to signal that
				display_symbol(LCD_ICON_RECORD, SEG_ON);
				start_buzzer(1, CONV_MS_TO_TICKS(20), CONV_MS_TO_TICKS(10));
				Timer0_A4_Delay(CONV_MS_TO_TICKS(30));
				stop_buzzer();
				display_symbol(LCD_ICON_RECORD, SEG_OFF);
			}

			doorlock_sequence_pause = 0;

			// is the sequence full?
			if (length <= DOORLOCK_SEQUENCE_MAX_LENGTH)
			{
				// start pause timer
				fptr_Timer0_A1_function = doorlock_sequence_pause_timer;
				Timer0_A1_Start(DOORLOCK_SEQUENCE_PAUSE_RESOLUTION);
				continue;
			}

			// if sequnce is full, we stop
			as_stop();
		}
		else
		{
			// if pause timeout we stop
			as_stop();

			doorlock_sequence_pause = 0;

			// is sequence too short?
			if (length <= DOORLOCK_SEQUENCE_MIN_LENGTH)
			{
				// reset data when exiting this state
				memset(sequence, 0, sizeof(u8) * DOORLOCK_SEQUENCE_MAX_LENGTH);
				// Reset IRQ flags
					BUTTONS_IFG &= ~ALL_BUTTONS;

					// Enable button interrupts
					BUTTONS_IE |= ALL_BUTTONS;
				return DOORLOCK_ERROR_FAILURE;
			}
		}

		// normalize all pauses
		ratio = 255.0f / (float)max;
		for (i = 0; i < DOORLOCK_SEQUENCE_MAX_LENGTH; i++)
		{
			sequence[i] *= ratio;
		}
		// Reset IRQ flags
			BUTTONS_IFG &= ~ALL_BUTTONS;

			// Enable button interrupts
			BUTTONS_IE |= ALL_BUTTONS;
		return DOORLOCK_ERROR_SUCCESS;
	}

}


// *************************************************************************************************
// @fn          doorlock_sequence_timer
// @brief       timer for timing out sequence input (if the user don't input unlock sequence for
//				a while, we want to shut down accelerometer to conserve power.
// @param       none
// @return      none
// *************************************************************************************************
void doorlock_sequence_timer(void)
{
	if (doorlock_sequence_timeout > 0)
	{
		-- doorlock_sequence_timeout;
	}
	else
	{
		Timer0_A1_Stop();
	}
}

// *************************************************************************************************
// @fn          doorlock_sequence_pause_timer
// @brief       timer callback to time the length of a pause
// @param       none
// @return      none
// *************************************************************************************************
void doorlock_sequence_pause_timer(void)
{
	if (doorlock_sequence_pause > DOORLOCK_SEQUENCE_PAUSE_MAX_LENGTH)
    {
            // stop timer
            Timer0_A1_Stop();
    }
    else
    {
            // Increment pause length
            ++ doorlock_sequence_pause;
    }
}

u8 sequence_compare(u8* sequence_a, u8* sequence_b)
{
	u8 i = 0;

	for (i = 0; i < DOORLOCK_SEQUENCE_MAX_LENGTH; i++)
    {
		if (sequence_a[i] > sequence_b[i])
		{
			if (sequence_a[i] - sequence_b[i] > DOORLOCK_SEQUENCE_SIMILARITY)
			{
				break;
			}
		}
		else
		{
			if (sequence_b[i] - sequence_a[i] > DOORLOCK_SEQUENCE_SIMILARITY)
			{
				break;
			}
		}
    }

	if (i == DOORLOCK_SEQUENCE_MAX_LENGTH)
	{
		//request.flag.lock_unlock = 1;
		//auth_signal_success();
		return DOORLOCK_ERROR_SUCCESS;
	}
	//auth_signal_failure();
		return DOORLOCK_ERROR_FAILURE;

}

#endif // CONFIG_USE_GPS