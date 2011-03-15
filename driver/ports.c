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
// Button entry functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "ports.h"
#include "buzzer.h"
#ifdef FEATURE_PROVIDE_ACCEL
#include "vti_as.h"
#endif
#include "vti_ps.h"
#include "timer.h"
#include "display.h"

// logic
#include "clock.h"
#include "alarm.h"
#include "rfsimpliciti.h"
#include "simpliciti.h"
#include "altitude.h"
#include "stopwatch.h"


// *************************************************************************************************
// Prototypes section
void button_repeat_on(u16 msec);
void button_repeat_off(void);
void button_repeat_function(void);


// *************************************************************************************************
// Defines section

// Macro for button IRQ 
#define IRQ_TRIGGERED(flags, bit)		((flags & bit) == bit)


// *************************************************************************************************
// Global Variable section
volatile s_button_flags button;
volatile struct struct_button sButton;


// *************************************************************************************************
// Extern section
extern void (*fptr_Timer0_A3_function)(void);


// *************************************************************************************************
// @fn          init_buttons
// @brief       Init and enable button interrupts.
// @param       none
// @return      none
// *************************************************************************************************
void init_buttons(void)
{
	// Set button ports to input 
	BUTTONS_DIR &= ~ALL_BUTTONS; 

	// Enable internal pull-downs
	BUTTONS_OUT &= ~ALL_BUTTONS; 
	BUTTONS_REN |= ALL_BUTTONS; 

	// IRQ triggers on rising edge
	BUTTONS_IES &= ~ALL_BUTTONS;   

	// Reset IRQ flags
	BUTTONS_IFG &= ~ALL_BUTTONS;  

	// Enable button interrupts
	BUTTONS_IE |= ALL_BUTTONS;   
}




// *************************************************************************************************
// @fn          PORT2_ISR
// @brief       Interrupt service routine for
//					- buttons 
//					- acceleration sensor CMA_INT 
//					- pressure sensor DRDY
// @param       none
// @return      none
// *************************************************************************************************
//pfs 
#ifdef __GNUC__  
#include <signal.h>
interrupt (PORT2_VECTOR) PORT2_ISR(void)
#else
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
#endif
{
	u8 int_flag, int_enable;
	u8 buzzer = 0;
	u8 simpliciti_button_event = 0;
	static u8 simpliciti_button_repeat = 0;

	// Clear button flags
	button.all_flags = 0;

	// Remember interrupt enable bits
	int_enable = BUTTONS_IE;

	// Store valid button interrupt flag
	int_flag = BUTTONS_IFG & int_enable;

	// ---------------------------------------------------
	// While SimpliciTI stack is active, buttons behave differently:
	//  - Store button events in SimpliciTI packet data
	//  - Exit SimpliciTI when button DOWN was pressed 
  	if (is_rf())
  	{
  		// Erase previous button press after a number of resends (increase number if link quality is low)
  		// This will create a series of packets containing the same button press
  		// Necessary because we have no acknowledge
  		// Filtering (edge detection) will be done by receiver software
  		if (simpliciti_button_repeat++ > 6) 
  		{
  			simpliciti_data[0] &= ~0xF0;
  			simpliciti_button_repeat = 0;
  		}
  		
  		if ((int_flag & BUTTON_STAR_PIN) == BUTTON_STAR_PIN)			
  		{
  			simpliciti_data[0] |= SIMPLICITI_BUTTON_STAR;
  			simpliciti_button_event = 1;
  		}
  		else if ((int_flag & BUTTON_NUM_PIN) == BUTTON_NUM_PIN)	
  		{
  			simpliciti_data[0] |= SIMPLICITI_BUTTON_NUM;
  			simpliciti_button_event = 1;
  		}
		else if ((int_flag & BUTTON_UP_PIN) == BUTTON_UP_PIN)	
		{
			simpliciti_data[0] |= SIMPLICITI_BUTTON_UP;
			simpliciti_button_event = 1;
		}
		else if ((int_flag & BUTTON_DOWN_PIN) == BUTTON_DOWN_PIN)	
		{
			simpliciti_flag |= SIMPLICITI_TRIGGER_STOP;
		}
		
		// Trigger packet sending inside SimpliciTI stack
		if (simpliciti_button_event) simpliciti_flag |= SIMPLICITI_TRIGGER_SEND_DATA;
  	}
  	else // Normal operation
  	{
		// Debounce buttons
		if ((int_flag & ALL_BUTTONS) != 0)
		{ 
			// Disable PORT2 IRQ
			__disable_interrupt();
			BUTTONS_IE = 0x00; 
			__enable_interrupt();
	
			// Debounce delay 1
			Timer0_A4_Delay(CONV_MS_TO_TICKS(BUTTONS_DEBOUNCE_TIME_IN));
	
			// Reset inactivity detection
			sTime.last_activity = sTime.system_time;
		}

		// ---------------------------------------------------
		// STAR button IRQ
		if (IRQ_TRIGGERED(int_flag, BUTTON_STAR_PIN))
		{
			// Filter bouncing noise 
			if (BUTTON_STAR_IS_PRESSED)
			{
				button.flag.star = 1;
				
				// Generate button click
				buzzer = 1;
			}
		}
		// ---------------------------------------------------
		// NUM button IRQ
		else if (IRQ_TRIGGERED(int_flag, BUTTON_NUM_PIN))
		{
			// Filter bouncing noise 
			if (BUTTON_NUM_IS_PRESSED)
			{
				button.flag.num = 1;
	
				// Generate button click
				buzzer = 1;
				
				if( !sys.flag.lock_buttons)
				{
					#ifdef CONFIG_STOP_WATCH
					// Faster reaction for stopwatch split button press
					if (is_stopwatch_run())
					{
						split_stopwatch();
						button.flag.num = 0;
						
					}
					#endif
				}
			}
		}
		// ---------------------------------------------------
		// UP button IRQ
		else if (IRQ_TRIGGERED(int_flag, BUTTON_UP_PIN))
		{
			// Filter bouncing noise 
			if (BUTTON_UP_IS_PRESSED)
			{
				button.flag.up = 1;
		
				// Generate button click
				buzzer = 1;
			}
		}
		// ---------------------------------------------------
		// DOWN button IRQ
		else if (IRQ_TRIGGERED(int_flag, BUTTON_DOWN_PIN))
		{
			// Filter bouncing noise 
			if (BUTTON_DOWN_IS_PRESSED)
			{
				button.flag.down = 1;
	
				// Generate button click
				buzzer = 1;
				
				if( !sys.flag.lock_buttons)
				{
					#ifdef CONFIG_STOP_WATCH
					// Faster reaction for stopwatch stop button press
					if (is_stopwatch_run())
					{
						stop_stopwatch();
						button.flag.down = 0;
					}
					// Faster reaction for stopwatch start button press
					else if (is_stopwatch_stop())
					{
						start_stopwatch();
						button.flag.down = 0;
					}
					#endif
				}
			}
		}
		// ---------------------------------------------------
		// B/L button IRQ
		else if (IRQ_TRIGGERED(int_flag, BUTTON_BACKLIGHT_PIN))
		{
			// Filter bouncing noise 
			if (BUTTON_BACKLIGHT_IS_PRESSED)
			{
				sButton.backlight_status = 1;
				sButton.backlight_timeout = 0;
				P2OUT |= BUTTON_BACKLIGHT_PIN;
				P2DIR |= BUTTON_BACKLIGHT_PIN;
				button.flag.backlight = 1;
			}
		}	
	}
	
	// Trying to lock/unlock buttons?
	if ((button.flag.num && button.flag.down) || (button.flag.star && button.flag.up))
	{
		// No buzzer output
		buzzer = 0;
		button.all_flags = 0;
	}

	// Generate button click when button was activated
	if (buzzer)
	{
		// Any button event stops active alarm
		#ifdef CONFIG_ALARM
		if (sAlarm.state == ALARM_ON) 
		{
			stop_alarm();
			button.all_flags = 0;
		}
		else 
		#endif
		if (!sys.flag.up_down_repeat_enabled && !sys.flag.no_beep)
		{
			start_buzzer(1, CONV_MS_TO_TICKS(20), CONV_MS_TO_TICKS(150));
		}
		
		// Debounce delay 2
		Timer0_A4_Delay(CONV_MS_TO_TICKS(BUTTONS_DEBOUNCE_TIME_OUT));
	}
	
	#ifdef FEATURE_PROVIDE_ACCEL
	// ---------------------------------------------------
	// Acceleration sensor IRQ
	if (IRQ_TRIGGERED(int_flag, AS_INT_PIN))
	{
		// Get data from sensor
		request.flag.acceleration_measurement = 1;
  	}
	#endif
	
  	// ---------------------------------------------------
	// Pressure sensor IRQ
	if (IRQ_TRIGGERED(int_flag, PS_INT_PIN)) 
	{
		// Get data from sensor
		request.flag.altitude_measurement = 1;
  	}
  	
  	// ---------------------------------------------------
  	// Safe long button event detection
  	if(button.flag.star || button.flag.num) 
	{
		// Additional debounce delay to enable safe high detection
		Timer0_A4_Delay(CONV_MS_TO_TICKS(BUTTONS_DEBOUNCE_TIME_LEFT));
	
		// Check if this button event is short enough
		if (BUTTON_STAR_IS_PRESSED) button.flag.star = 0;
		if (BUTTON_NUM_IS_PRESSED) button.flag.num = 0;	
	}
	
	// Reenable PORT2 IRQ
	__disable_interrupt();
	BUTTONS_IFG = 0x00; 	
	BUTTONS_IE  = int_enable; 	
	__enable_interrupt();

	// Exit from LPM3/LPM4 on RETI
	__bic_SR_register_on_exit(LPM4_bits); 
}


// *************************************************************************************************
// @fn          button_repeat_on
// @brief       Start button auto repeat timer.
// @param       none
// @return      none
// *************************************************************************************************
void button_repeat_on(u16 msec)
{
	// Set button repeat flag
	sys.flag.up_down_repeat_enabled = 1;
	
	// Set Timer0_A3 function pointer to button repeat function
	fptr_Timer0_A3_function = button_repeat_function;
	
	// Timer0_A3 IRQ triggers every 200ms
	Timer0_A3_Start(CONV_MS_TO_TICKS(msec));
}


// *************************************************************************************************
// @fn          button_repeat_off
// @brief       Stop button auto repeat timer.
// @param       none
// @return      none
// *************************************************************************************************
void button_repeat_off(void)
{
	// Clear button repeat flag
	sys.flag.up_down_repeat_enabled = 0;
	
	// Timer0_A3 IRQ repeats with 4Hz
	Timer0_A3_Stop();
}


// *************************************************************************************************
// @fn          button_repeat_function
// @brief       Check at regular intervals if button is pushed continuously 
//				and trigger virtual button event.
// @param       none
// @return      none
// *************************************************************************************************
void button_repeat_function(void)
{
	static u8 start_delay = 10;	// Wait for 2 seconds before starting auto up/down
	u8 repeat = 0;
	
	// If buttons UP or DOWN are continuously high, repeatedly set button flag
	if (BUTTON_UP_IS_PRESSED)
	{
		if (start_delay == 0)
		{
			// Generate a virtual button event
			button.flag.up = 1;
			repeat = 1;
		}
		else
		{
			start_delay--;
		}
	}
	else if (BUTTON_DOWN_IS_PRESSED)
	{
		if (start_delay == 0)
		{
			// Generate a virtual button event
			button.flag.down = 1;
			repeat = 1;
		}
		else
		{
			start_delay--;
		}
	}
	else
	{
		// Reset repeat counter
		sButton.repeats = 0;
		start_delay = 10;

		// Enable blinking
		start_blink();
	}
	
	// If virtual button event is generated, stop blinking and reset timeout counter
	if (repeat)
	{
		// Increase repeat counter
		sButton.repeats++;

		// Reset inactivity detection counter
		sTime.last_activity = sTime.system_time;
		
		// Disable blinking
		stop_blink();
	}
}

