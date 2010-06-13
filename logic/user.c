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
// Several user functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"
#include "buzzer.h"
#include "ports.h"

// logic
#include "menu.h"
#include "date.h"
#include "clock.h"
#include "user.h"
#include "stopwatch.h"


// *************************************************************************************************
// Prototypes section


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section


// *************************************************************************************************
// Extern section
extern void idle_loop(void);


// *************************************************************************************************
// @fn          dummy
// @brief       Dummy direct function.
// @param       u8 line	LINE1, LINE2
// @return      none
// *************************************************************************************************
void dummy(u8 line)
{
}

 
// *************************************************************************************************
// @fn          set_value
// @brief       Generic value setting routine
// @param       s32 * value						Pointer to value to set
//				u8digits						Number of digits
//				u8 blanks						Number of whitespaces before first valid digit
//				s32 limitLow					Lower limit of value
//				s32 limitHigh					Upper limit of value
//				u16 mode		
//				u8 segments					Segments where value should be drawn
//				fptr_setValue_display_function1		Value-specific display routine
// @return      none
// *************************************************************************************************
void set_value(s32 * value, u8 digits, u8 blanks, s32 limitLow, s32 limitHigh, u16 mode, u8 segments, void (*fptr_setValue_display_function1)(u8 segments, u32 value, u8 digits, u8 blanks))
{
	u8 update;
	s16 stepValue = 1;
	u8 doRound = 0;
	u8 stopwatch_state;
	u32 val;
	
	// Clear button flags
	button.all_flags = 0;
	
	// Clear blink memory
	clear_blink_mem();
	
	// For safety only - buzzer on/off and button_repeat share same IRQ
	stop_buzzer();
	
	// Disable stopwatch display update while function is active
	stopwatch_state = sStopwatch.state;
	sStopwatch.state = STOPWATCH_HIDE;
	
	// Init step size and repeat counter
	sButton.repeats = 0;
	
	// Initial display update
	update = 1;
	
	// Turn on 200ms button repeat function 
	button_repeat_on(200);
	
	// Start blinking with with 2Hz
	set_blink_rate(BIT6 + BIT5);
	
	// Value set loop
	while(1) 
	{
		// Idle timeout: exit function
		if (sys.flag.idle_timeout) break;

		// Button STAR (short) button: exit function
		if (button.flag.star) break;

		// NUM button: exit function and goto to next value (if available)
		if (button.flag.num)
		{
			if ((mode & SETVALUE_NEXT_VALUE) == SETVALUE_NEXT_VALUE) break;
		}

		// UP button: increase value
		if(button.flag.up)
		{
			// Increase value
			* value = * value + stepValue;
			
			// Check value limits
			if (* value > limitHigh) 
			{
				// Check if value can roll over, else stick to limit
				if ((mode & SETVALUE_ROLLOVER_VALUE) == SETVALUE_ROLLOVER_VALUE) 	* value = limitLow;
				else 																* value = limitHigh;				
					
				// Reset step size to default
				stepValue = 1;	
			}

			// Trigger display update
			update = 1;
			
			// Clear button flag
			button.flag.up = 0;
		}
		
		// DOWN button: decrease value
		if(button.flag.down)
		{
			// Decrease value
			* value = * value - stepValue;
			
			// Check value limits
			if (* value < limitLow) 
			{
				// Check if value can roll over, else stick to limit
				if ((mode & SETVALUE_ROLLOVER_VALUE) == SETVALUE_ROLLOVER_VALUE)	* value = limitHigh;
				else																* value = limitLow;
					
				// Reset step size to default
				stepValue = 1;	
			}

			// Trigger display update
			update = 1;

			// Clear button flag	
			button.flag.down = 0;
		}

		
		// When fast mode is enabled, increase step size if Sx button is continuously
		if ((mode & SETVALUE_FAST_MODE) == SETVALUE_FAST_MODE)
		{
			switch (sButton.repeats)
			{
				case 0:			stepValue = 1;		doRound = 0; 	break;
				case 10:		
				case -10:		stepValue = 10;		doRound = 1; 	break;
				case 20:			
				case -20:		stepValue = 100;	doRound = 1; 	break;
				case 30:			
				case -30:		stepValue = 1000;	doRound = 1; 	break;
			}
			
			// Round value to avoid odd numbers on display
			if (stepValue != 1 && doRound == 1)	
			{
				* value -= * value % stepValue;
				doRound = 0;
			}
		}

		// Update display when there is new data
		if (update)
		{
			// Display up or down arrow according to sign of value
			if ((mode & SETVALUE_DISPLAY_ARROWS) == SETVALUE_DISPLAY_ARROWS)
			{
				if (* value >= 0)
				{
					display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
					val = *value;
				}
				else 
				{
					display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
					val = *value * (-1);
				}
			}
			else
			{
				val = *value;
			}

			// Display function can either display value directly, modify value before displaying 
			// or display a string referenced by the value
			fptr_setValue_display_function1(segments, val, digits, blanks);

			// Clear update flag
			update = 0;
		}
		
		// Call idle loop to serve background tasks
		idle_loop();
		
	}
	
	// Clear up and down arrows
	display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
	display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
	
	// Set blinking rate to 1Hz and stop
	set_blink_rate(BIT7 + BIT6 + BIT5);
	clear_blink_mem();
	
	// Turn off button repeat function
	button_repeat_off();

	// Enable stopwatch display updates again
	sStopwatch.state = stopwatch_state;
}
 
