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
// eggtimer functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"
#include <string.h>

// driver
#include "eggtimer.h"
#include "ports.h"
#include "display.h"
#include "timer.h"
#include "buzzer.h"
#include "user.h"

// logic
#include "menu.h"


// *************************************************************************************************
// Prototypes section
void start_eggtimer(void);
void stop_eggtimer(void);
void reset_eggtimer(void);
void eggtimer_tick(void);
void update_eggtimer_timer(void);
void mx_eggtimer(u8 line);
void sx_eggtimer(u8 line);
void display_eggtimer(u8 line, u8 update);
extern void set_eggtimer(void);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct eggtimer seggtimer;


// *************************************************************************************************
// Extern section


// *************************************************************************************************
// @fn          update_eggtimer_timer
// @brief       Set new compare time for next 1/1Hz or 1/100Hz interrupt. Takes care for exact 1 second timing.
// @param       ticks (1 tick = 1/32768 sec)
// @return      none
// *************************************************************************************************
void update_eggtimer_timer(void)
{
    //Make sure eggtimer is running before we do anything else (this might slightly mess up my timing, but oh well)
  if(seggtimer.state == EGGTIMER_RUN){
	u16 value;

	// Load CCR register with next capture time
	if (seggtimer.viewStyle == DISPLAY_DEFAULT_VIEW) 
	{
		// Timer interrupts occur every 32768/100 = 328 ACLK
		// --> eggtimer runs too slow (1 sec nominal != 100 interupts * 328 ACLK = 32800 ACLK = 1.00098 sec)
		// --> ideally correct timer value every 10 ticks by (32768 - 32800)/10 = 3.2
		// --> correct timer value every 10Hz by 3, 
		// --> correct timer value every 1Hz correct by 5
		value = TA0CCR2 + EGGTIMER_100HZ_TICK;

		if (seggtimer.swtIs1Hz) 
		{
			value -= 5;
			seggtimer.swtIs1Hz = 0;	
			seggtimer.swtIs10Hz = 0;	
		}
		else if (seggtimer.swtIs10Hz) 
		{
			value -= 3;
			seggtimer.swtIs10Hz = 0;	
		}
	}
	else // Alternative view
	{
		// Timer interrupts occur every 32768/1 = 32768 ACLK
		value = TA0CCR2 + EGGTIMER_1HZ_TICK;
	}
	
	// Update CCR
	TA0CCR2 = value;   
  }
}


// *************************************************************************************************
// @fn          eggtimer_tick
// @brief       Called by 1/100Hz interrupt handler. 
//				Decreases eggtimer counter and triggers display update.
// @param       none
// @return      none
// *************************************************************************************************
void eggtimer_tick(void)
{
  //Make sure eggtimer is running before we do anything else (this might slightly mess up my timing, but oh well)
  if(seggtimer.state == EGGTIMER_RUN){
	static u8 delay = 0;
	
	// Default view (< 20 minutes): display and count MM:SS:hh
	if (seggtimer.viewStyle == DISPLAY_DEFAULT_VIEW)
	{
		// Add 1/100 sec 
		seggtimer.time[7]--;
				
		// Draw flag minimizes display update activity
		//
		// swt.drawFlag = 1: second L
		// swt.drawFlag = 2: second H/L
		// swt.drawFlag = 3: minutes L, second H/L
		// swt.drawFlag = 4: minutes H/L, second H/L
		// swt.drawFlag = 5: hours L, minutes H/L, second H/L
		// swt.drawFlag = 6: hours H/L, minutes H/L, second H/L
		// swt.drawFlag = 7: 1/10 sec, 1/100 sec
		// swt.drawFlag = 8: 1/100 sec (every 17/100 sec to reduce display draw activity)
		if (delay++ > 17) 
		{
			seggtimer.drawFlag = 8;
			delay = 0;
		}
	
		// Subtract 1/10 sec 
		if (seggtimer.time[7] == 0x2F)
		{
			seggtimer.time[7]='9';
			seggtimer.time[6]--;
			
			// 1/10Hz trigger 
			seggtimer.swtIs10Hz = 1;
			
			// Update draw flag
			seggtimer.drawFlag = 7;
		}
	}
	else // Alternative view (20 minutes .. 20 hours): display and count HH:MM:SS
	{
		// Just subtract 1 second
		seggtimer.time[6] = 0x2F;
	}
			
	// Second overflow?
	if (seggtimer.time[6] == 0x2F)
	{
		// Reset draw flag
		seggtimer.drawFlag = 1;

		// 1Hz trigger 
		seggtimer.swtIs1Hz = 1;
		
		// Subtract sequentially
		seggtimer.time[6]='9';
		seggtimer.time[5]--;							// second  L (0 - 9)
		if (seggtimer.time[5] == 0x2F) 
		{
			seggtimer.drawFlag++;						// 2
			seggtimer.time[5] = '9';
			seggtimer.time[4]--;						// second  H (0 - 5)
			if (seggtimer.time[4] == 0x2F) 
			{
				seggtimer.drawFlag ++;					// 3
				seggtimer.time[4] = '5';
				seggtimer.time[3]--;					// minutes L (0 - 9)
				if (seggtimer.time[3] == 0x2F) 
				{
					seggtimer.drawFlag++;				// 4
					seggtimer.time[3] = '9';
					seggtimer.time[2]--;				// minutes H (0 - 5)
					
                                        if (seggtimer.time[0]=='0'&&
                                            seggtimer.time[1]=='0'&&
                                            seggtimer.time[2]< '2')
                                        {
	                                        // SWT display changes from HH:MM:SS to MM:SS:hh when reaching 20 minutes 
	                                        seggtimer.viewStyle = DISPLAY_DEFAULT_VIEW;
	                                        display_eggtimer(LINE2, DISPLAY_LINE_UPDATE_FULL);
	                                } 
                                        
					if (seggtimer.time[2] == 0x2F) 
					{
						seggtimer.drawFlag++;				// 5
						seggtimer.time[2] = '5';
						seggtimer.time[1]--;				// hours L (0-9)	

						if (seggtimer.time[1] == 0x2F) 
						{
							seggtimer.drawFlag++;			// 6
							seggtimer.time[1] = '9';
							seggtimer.time[0]--;			// hours H (0-1)	

		   				} 
                                                if(seggtimer.time[0] ==0x2F)
                                                  {
                                                          // When we reach 0, stop, reset, and beep (not able to stop beeping yet) 
                                                          stop_eggtimer();
                                                          reset_eggtimer();
                                                          display_eggtimer(LINE2, DISPLAY_LINE_UPDATE_FULL);
                                                          start_buzzer(100, CONV_MS_TO_TICKS(20), CONV_MS_TO_TICKS(150));
                                                  }
	   				} 
				}
			}
		}
	}		
        
	// Always set display update flag (Only used in two places for no good reason, but whatever)
        //seggtimer.update_eggtimer = 1;
        display.flag.update_stopwatch = 1;
  }
}



// *************************************************************************************************
// @fn          reset_eggtimer
// @brief       Clears eggtimer counter and sets eggtimer state to off.
// @param       none
// @return      none
// *************************************************************************************************
void reset_eggtimer(void)
{
	// Clear counter
	memcpy(seggtimer.time, seggtimer.defaultTime, sizeof(seggtimer.time));

	// Clear trigger
	seggtimer.swtIs10Hz 	= 0;		// 1/10Hz trigger
	seggtimer.swtIs1Hz  	= 0;		// 1Hz trigger
	
	// Init eggtimer state 'Off'
	seggtimer.state 	  	= EGGTIMER_STOP;		
	
	// Default display style is MM:SS:HH
        if(seggtimer.defaultTime[0]=='0'&&
           seggtimer.defaultTime[1]=='0'&&
           seggtimer.defaultTime[2]<'2')
              seggtimer.viewStyle 	= DISPLAY_DEFAULT_VIEW;
        else
	      seggtimer.viewStyle 	= DISPLAY_ALTERNATIVE_VIEW;
}


// *************************************************************************************************
// @fn          is_eggtimer
// @brief       Is eggtimer operating and visible?
// @param       none
// @return      1=EGGTIMER_RUN, 0=other states
// *************************************************************************************************
u8 is_eggtimer(void)
{
	return ((seggtimer.state == EGGTIMER_RUN) && (ptrMenu_L2 == &menu_L2_Eggtimer));
}


// *************************************************************************************************
// @fn          start_eggtimer
// @brief       Starts eggtimer timer interrupt and sets eggtimer state to on.
// @param       none
// @return      none
// *************************************************************************************************
void start_eggtimer(void)
{
	// Set eggtimer run flag
	seggtimer.state = EGGTIMER_RUN;	

	// Init CCR register with current time
	TA0CCR2 = TA0R;
		
	// Load CCR register with next capture time
	update_eggtimer_timer();

	// Reset IRQ flag    
	TA0CCTL2 &= ~CCIFG; 
	          
	// Enable timer interrupt    
	TA0CCTL2 |= CCIE; 
	
	// Set eggtimer icon (doesn't exist so I wont untill I'll use stopwatch for now)
	display_symbol(LCD_ICON_RECORD, SEG_ON);
}


// *************************************************************************************************
// @fn          stop_eggtimer
// @brief       Stops eggtimer timer interrupt and sets eggtimer state to off.
//				Does not reset eggtimer count.
// @param       none
// @return      none
// *************************************************************************************************
void stop_eggtimer(void)
{
	// Clear timer interrupt enable   
	TA0CCTL2 &= ~CCIE; 

	// Clear eggtimer run flag
	seggtimer.state = EGGTIMER_STOP;	
	
	// Clear eggtimer icon (doesn't exist so I'll use stopwatch for now)
	display_symbol(LCD_ICON_RECORD, SEG_OFF);

	// Call draw routine immediately
	display_eggtimer(LINE2, DISPLAY_LINE_UPDATE_FULL);
}


// *************************************************************************************************
// @fn          mx_eggtimer
// @brief       eggtimer set routine. Mx stops eggtimer and resets count.
// @param       u8 line	LINE2
// @return      none
// *************************************************************************************************
void mx_eggtimer(u8 line)
{
	// Stop eggtimer
	stop_eggtimer();
        
        //Set eggtimer
        set_eggtimer();
			
	// Reset eggtimer count
	reset_eggtimer();	
	
	// Display eggtimer time
	display_eggtimer(line, DISPLAY_LINE_UPDATE_FULL);
        
        
}


// *************************************************************************************************
// @fn          sx_eggtimer
// @brief       eggtimer direct function. S2 starts/stops eggtimer, but does not reset count.
// @param       u8 line	LINE2
// @return      none
// *************************************************************************************************
void sx_eggtimer(u8 line)
{
	// S2: RUN, STOP
	if(button.flag.s2)
	{
		if (seggtimer.state == EGGTIMER_STOP)
		{
			// (Re)start eggtimer
			start_eggtimer();
		}
		else 
		{
			// Stop eggtimer 
			stop_eggtimer();
		}
			
	}
}


// *************************************************************************************************
// @fn          display_eggtimer
// @brief       eggtimer user routine. Sx starts/stops eggtimer, but does not reset count.
// @param       u8 line	LINE2
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
void display_eggtimer(u8 line, u8 update)
{
	// Partial line update only
	if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{	
//		if (seggtimer.update_eggtimer)
                if (display.flag.update_stopwatch)
		{
			if (seggtimer.viewStyle == DISPLAY_DEFAULT_VIEW)
			{
				// Display MM:SS:hh

				// Check draw flag to minimize workload
				if(seggtimer.drawFlag != 0) 
				{
					switch(seggtimer.drawFlag) 
					{
						case 4: display_char(LCD_SEG_L2_5, seggtimer.time[2], SEG_ON);
						case 3: display_char(LCD_SEG_L2_4, seggtimer.time[3], SEG_ON);
						case 2: display_char(LCD_SEG_L2_3, seggtimer.time[4], SEG_ON);
						case 1: display_char(LCD_SEG_L2_2, seggtimer.time[5], SEG_ON);
						case 7: display_char(LCD_SEG_L2_1, seggtimer.time[6], SEG_ON);
						case 8:	display_char(LCD_SEG_L2_0, seggtimer.time[7], SEG_ON);
					}
				}
			}
			else // DISPLAY_ALTERNATIVE_VIEW
			{
				// Display HH:MM:SS
				switch(seggtimer.drawFlag) 
				{
					case 6: display_char(LCD_SEG_L2_5, seggtimer.time[0], SEG_ON);
					case 5: display_char(LCD_SEG_L2_4, seggtimer.time[1], SEG_ON);
					case 4: display_char(LCD_SEG_L2_3, seggtimer.time[2], SEG_ON);
					case 3: display_char(LCD_SEG_L2_2, seggtimer.time[3], SEG_ON);
					case 2: display_char(LCD_SEG_L2_1, seggtimer.time[4], SEG_ON);
					case 1: display_char(LCD_SEG_L2_0, seggtimer.time[5], SEG_ON);
				}		
			}
		}
	}
	// Redraw whole line
	else if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		if (seggtimer.viewStyle == DISPLAY_DEFAULT_VIEW)
		{
			// Display MM:SS:hh
			display_chars(LCD_SEG_L2_5_0, seggtimer.time+2, SEG_ON);
		}
		else // DISPLAY_ALTERNATIVE_VIEW
		{
			// Display HH:MM:SS
			display_chars(LCD_SEG_L2_5_0, seggtimer.time, SEG_ON);
		}
		display_symbol(LCD_SEG_L2_COL1, SEG_ON);
		display_symbol(LCD_SEG_L2_COL0, SEG_ON);
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		// Clean up symbols when leaving function
	}
}

// *************************************************************************************************
// @fn          set_eggtimer
// @brief       Set's the eggtimer. (Almost entirely copied from the alarm section
// @param       none
// @return      none
// *************************************************************************************************
extern void set_eggtimer(void){
        u8 select;
	s32 hours;
	s32 minutes;
        s32 seconds;
        u8 * str;
        
        //Get hours, minutes, and seconds into interger form
        hours   = (seggtimer.defaultTime[0]-48)*10  + (seggtimer.defaultTime[1]-48);
        minutes = (seggtimer.defaultTime[2]-48)*10  + (seggtimer.defaultTime[3]-48);
        seconds = (seggtimer.defaultTime[4]-48)*10  + (seggtimer.defaultTime[5]-48);
        
        // Display HH:MM:SS (LINE2) 
	str = itoa(hours, 2, 0);
	display_chars(LCD_SEG_L2_5_4, str, SEG_ON);
	display_symbol(LCD_SEG_L2_COL0, SEG_ON);
	
	str = itoa(minutes, 2, 0);
	display_chars(LCD_SEG_L2_3_2, str, SEG_ON);
        
        str = itoa(seconds, 2, 0);
        display_chars(LCD_SEG_L2_1_0, str, SEG_ON);
        
        // Init value index
	select = 0;	
		
	// Loop values until all are set or user breaks	set
	while(1) 
	{
		// Idle timeout: exit without saving 
		if (sys.flag.idle_timeout) break;
		
		// M2 (short): save, then exit 
		if (button.flag.m2) 
		{
			// Store local variables in global Eggtimer default
			//sAlarm.hour = hours;
			//sAlarm.minute = minutes;
                        seggtimer.defaultTime[0]=*itoa(hours, 2, 0);
                        seggtimer.defaultTime[1]=*itoa(hours, 1, 0);
                        seggtimer.defaultTime[2]=*itoa(minutes, 2, 0);
                        seggtimer.defaultTime[3]=*itoa(minutes, 1, 0);
                        seggtimer.defaultTime[4]=*itoa(seconds, 2, 0);
                        seggtimer.defaultTime[5]=*itoa(seconds, 1, 0);
		        //Set display update flag
			display.flag.line2_full_update = 1;
			break;
		}

		switch (select)
		{
			case 0:		// Set hours
                                        set_value(&hours, 2, 0, 0, 19, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_5_4, display_value1);
                                        select = 1;
                                        break;

			case 1:		// Set minutes
                                        set_value(&minutes, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_3_2, display_value1);
                                        select = 2;
                                        break;
                                        
                        case 2:		// Set seconds
                                        set_value(&seconds, 2, 0, 0, 59, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
                                        select = 0;
                                        break;
		}
	}
	
	// Clear button flag
	button.all_flags = 0;
	
}