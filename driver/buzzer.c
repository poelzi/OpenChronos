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
// Buzzer functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "buzzer.h"
#include "timer.h"
#include "display.h"

// logic
#include "alarm.h"


// *************************************************************************************************
// Prototypes section
void toggle_buzzer(void);
void countdown_buzzer(void);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct buzzer sBuzzer;
 

// *************************************************************************************************
// Extern section
//extern u16 timer0_A3_ticks_g;



// *************************************************************************************************
// @fn          reset_buzzer
// @brief       Init buzzer variables
// @param       none
// @return      none
// *************************************************************************************************
void reset_buzzer(void)
{
	sBuzzer.time 	= 0;
	sBuzzer.state 	= BUZZER_OFF;
}

// *************************************************************************************************
// @fn          start_buzzer
// @brief       Start buzzer output for a number of cylces
// @param       u8 cycles		Keep buzzer output for number of cycles
//				u16 on_time	Output buzzer for "on_time" ACLK ticks
//				u16 off_time	Do not output buzzer for "off_time" ACLK ticks
// @return      none
// *************************************************************************************************
void start_buzzer(u8 cycles, u16 on_time, u16 off_time)
{
	// Store new buzzer duration while buzzer is off
	if (sBuzzer.time == 0) 
	{
		sBuzzer.time 	 = cycles;
		sBuzzer.on_time  = on_time;
		sBuzzer.off_time = off_time;

		// Need to init every time, because SimpliciTI claims same timer
			
		// Reset TA1R, set up mode, TA1 runs from 32768Hz ACLK 
		TA1CTL = TACLR | MC_1 | TASSEL__ACLK;

		// Set PWM frequency 
		TA1CCR0 = BUZZER_TIMER_STEPS;		

		// Enable IRQ, set output mode "toggle"
		TA1CCTL0 = OUTMOD_4;

		// Allow buzzer PWM output on P2.7
		P2SEL |= BIT7;

		// Activate Timer0_A3 periodic interrupts
		fptr_Timer0_A3_function = toggle_buzzer;
		Timer0_A3_Start(sBuzzer.on_time);

		// Preload timer advance variable
		sTimer.timer0_A3_ticks = sBuzzer.off_time;

		// Start with buzzer output on
		sBuzzer.state 	 	= BUZZER_ON_OUTPUT_ENABLED;
	}
}



// *************************************************************************************************
// @fn          toggle_buzzer
// @brief       Keeps track of buzzer on/off duty cycle
// @param       none
// @return      none
// *************************************************************************************************
void toggle_buzzer(void)
{
	// Turn off buzzer
	if (sBuzzer.state == BUZZER_ON_OUTPUT_ENABLED)
	{
		// Stop PWM timer 
		TA1CTL &= ~(BIT4 | BIT5);

		// Reset and disable buzzer PWM output
		P2OUT &= ~BIT7;
		P2SEL &= ~BIT7;
		
		// Update buzzer state
		sBuzzer.state = BUZZER_ON_OUTPUT_DISABLED;
		
		// Reload Timer0_A4 IRQ to restart output
		sTimer.timer0_A3_ticks = sBuzzer.on_time;
	}
	else // Turn on buzzer
	{
		// Decrement buzzer total cycles
		countdown_buzzer();
		
		// Reload Timer0_A4 to stop output if sBuzzer.time > 0
		if (sBuzzer.state != BUZZER_OFF) 
		{
			// Reset timer TA1
			TA1R = 0;
			TA1CTL |= MC_1;

			// Enable buzzer PWM output
			P2SEL |= BIT7;
	
			// Update buzzer state
			sBuzzer.state = BUZZER_ON_OUTPUT_ENABLED;
	
			// Reload Timer0_A4 IRQ to turn off output
			sTimer.timer0_A3_ticks = sBuzzer.off_time;
		}
	}
}


// *************************************************************************************************
// @fn          stop_buzzer
// @brief       Stop buzzer output
// @param       none
// @return      none
// *************************************************************************************************
void stop_buzzer(void)
{
	// Stop PWM timer 
	TA1CTL &= ~(BIT4 | BIT5);

	// Disable buzzer PWM output
	P2OUT &= ~BIT7;
	P2SEL &= ~BIT7;
	
	// Clear PWM timer interrupt    
	TA1CCTL0 &= ~CCIE; 

	// Disable periodic start/stop interrupts
	Timer0_A3_Stop();

	// Clear variables
	reset_buzzer();
}



// *************************************************************************************************
// @fn          is_buzzer
// @brief       Check if buzzer is operating
// @param       none
// @return      u8		1 = Buzzer is operating, 0 = Buzzer is off
// *************************************************************************************************
u8 is_buzzer(void)
{
	return (sBuzzer.state != BUZZER_OFF);
}



// *************************************************************************************************
// @fn          countdown_buzzer
// @brief       Decrement active buzzer time. Turn off buzzer if cycle end reached.
// @param       none
// @return      none
// *************************************************************************************************
void countdown_buzzer(void)
{
	// Stop buzzer when reaching 0 cycles
	if (--sBuzzer.time == 0)
	{
		stop_buzzer();
	}
}
