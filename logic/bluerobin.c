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
// BlueRobin functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"
#include "radio.h"
#include "ports.h"
#include "timer.h"
#include "rf1a.h"

// logic
#include "BlueRobin_RX_API.h"
#include "bluerobin.h"
#include "rfsimpliciti.h"
#include "user.h"


// *************************************************************************************************
// Prototypes section
void display_calories(u8 line, u8 update);
void display_distance(u8 line, u8 update);


// *************************************************************************************************
// Defines section

// Set to TRUE if transmitter ID should be remembered when reconnecting 
// Transmitter ID can be cleared by pressing button STAR for more than 3 seconds
#define REMEMBER_TX_ID			(FALSE)


// *************************************************************************************************
// Global Variable section
struct br sBlueRobin;

// Display values for user sex selection
const u8 selection_User_Sex[][4] = { "MALE", "FEMA" };


// *************************************************************************************************
// Extern section

// Stop BlueRobin timer
extern void BRRX__StopTimer_v(void);

// Calibration value for FSCTRL0 register (corrects deviation of 26MHz crystal)
extern u8 rf_frequoffset;


// *************************************************************************************************
// @fn          reset_bluerobin
// @brief       Reset BlueRobin data. 
// @param       none
// @return      none
// *************************************************************************************************
void reset_bluerobin(void)
{
	// Reset state is no connection
	sBlueRobin.state = BLUEROBIN_OFF;
	
	// Reset value of chest strap ID is 0 --> connect to next best chest strap
	sBlueRobin.cs_id 		= 0;
	
	// No new data available
	sBlueRobin.update 		= BLUEROBIN_NO_UPDATE;
	sBlueRobin.heartrate 	= 0;
	sBlueRobin.speed 		= 0;
	sBlueRobin.distance 	= 0;
	sBlueRobin.calories 	= 0;
	
	// Set user data to default
	sBlueRobin.user_sex 	= USER_SEX_MALE;
	sBlueRobin.user_weight 	= 75;
	
	// Display calories as default
	sBlueRobin.caldist_view = 0;
}



// *************************************************************************************************
// @fn          mx_rfblue
// @brief       BlueRobin sub menu. Button STAR resets chest strap ID to 0 and searches for next chest strap in range.
// @param       u8 line	LINE2
// @return      none
// *************************************************************************************************
void mx_bluerobin(u8 line)
{
#if REMEMBER_TX_ID == TRUE
  u8 i;

	// Reset chest strap ID
	sBlueRobin.cs_id = 0;

  display_chars(LCD_SEG_L1_2_0, (u8*)"CLR", SEG_ON);
  for (i=0; i<4; i++) Timer0_A4_Delay(CONV_MS_TO_TICKS(500));
#endif

	// Clear simulated button event
	button.all_flags = 0;
}



// *************************************************************************************************
// @fn          sx_bluerobin
// @brief       BlueRobin direct function. Button UP connects/disconnects with sender unit.
// @param       u8 line		LINE1
// @return      none
// *************************************************************************************************
void sx_bluerobin(u8 line)
{
	u8 stop = 0;
	
	// Exit if battery voltage is too low for radio operation
	if (sys.flag.low_battery) return;
	
	// Exit if SimpliciTI stack is active
	if (is_rf()) return;
		
	// UP: connect / disconnect transmitter
	if(button.flag.up)
	{
		if (sBlueRobin.state == BLUEROBIN_OFF)
		{
			// Init BlueRobin timer and radio
			open_radio();

			// Initialize BR library
			BRRX_Init_v();
			  
			// Set BR data transmission properties
			BRRX_SetPowerdownDelay_v(10);       // Power down channel after 10 consecutive lost data packets (~9 seconds)
			BRRX_SetSearchTimeout_v(8);         // Stop searching after 8 seconds
			
			// Sensitivity in learn mode reduced --> connect only to close transmitters
			// Skip this part if chest strap id was set in a previous learn mode run
#if REMEMBER_TX_ID == TRUE
			if (sBlueRobin.cs_id == 0) BRRX_SetSignalLevelReduction_v(5);

#else
			// Forget previously learned transmitter ID and connect to next close transmitter
			sBlueRobin.cs_id = 0;
			BRRX_SetSignalLevelReduction_v(5);  	
#endif

			// Apply frequency offset compensation to radio register FSCTRL0
			// If calibration memory was erased, rf_frequoffset defaults to 0x00 and has no effect
			WriteSingleReg(FSCTRL0, rf_frequoffset);

			// New state is SEARCH			
			sBlueRobin.state = BLUEROBIN_SEARCHING;	

			// Blink RF icon to show searching
			display_symbol(LCD_ICON_BEEPER1, SEG_ON_BLINK_ON);
			display_symbol(LCD_ICON_BEEPER2, SEG_ON_BLINK_ON);
			display_symbol(LCD_ICON_BEEPER3, SEG_ON_BLINK_ON);

			// Turn on radio and establish connection if channel not already started
			if (BRRX_GetState_t(HR_CHANNEL) == TX_OFF)
			{
				// Start in learn mode (connect to closest heart rate transmitter)
				BRRX_SetID_v(HR_CHANNEL, sBlueRobin.cs_id);
				BRRX_Start_v(HR_CHANNEL);

				// Wait until learning phase is over
			 	while (BRRX_GetState_t(HR_CHANNEL)==TX_SEARCH) 
			 	{
					Timer0_A4_Delay(CONV_MS_TO_TICKS(200));
    			}
			}
			
		 	// Check if connection attempt was successful
			if (BRRX_GetState_t(HR_CHANNEL)==TX_ACTIVE)	
			{
				// Successfully connected to transmitter
				sBlueRobin.state = BLUEROBIN_CONNECTED;		
				
				// When in learn mode, copy chest strap ID
				if (sBlueRobin.cs_id == 0) 
				{
					sBlueRobin.cs_id = BRRX_GetID_u32(HR_CHANNEL);
				}

				// Show steady RF icon to indicate established connection
				display_symbol(LCD_ICON_BEEPER1, SEG_ON_BLINK_OFF);
				display_symbol(LCD_ICON_BEEPER2, SEG_ON_BLINK_OFF);
				display_symbol(LCD_ICON_BEEPER3, SEG_ON_BLINK_OFF);
				// Show blinking icon
				display_symbol(LCD_ICON_HEART, SEG_ON_BLINK_ON);
			}
			else // Error -> Shutdown connection
			{
			  	stop = 1;
			}			
		}
		else if (sBlueRobin.state == BLUEROBIN_CONNECTED)
		{
			// Shutdown connection
			stop = 1;
		}
	}	
	
	// Shutdown connection
	if (stop)
	{
		stop_bluerobin();		
	}
}


// *************************************************************************************************
// @fn          display_selection_User_Sex
// @brief        
// @param       
//				
// @return      none
// *************************************************************************************************
void display_selection_User_Sex1(u8 segments, u32 index, u8 digits, u8 blanks, u8 dummy)
{
	if (index < 2) display_chars(segments, (u8 *)selection_User_Sex[index], SEG_ON_BLINK_ON);
}


// *************************************************************************************************
// @fn          mx_caldist
// @brief       Calories/Distance sub menu. Mx enables setting of total calories, user sex and weight.
// @param       u8 line		LINE2
// @return      none
// *************************************************************************************************
void mx_caldist(u8 line)
{
	u8 select;
	s32 kcalories;
	s32 weight;
	s32 sex;

	// Clear display
	clear_display_all();
	
	// Convert global variables to local variables
	sex 		= sBlueRobin.user_sex;
	kcalories 	= sBlueRobin.calories/1000;
	if (sys.flag.use_metric_units)	weight = sBlueRobin.user_weight;
	else							weight = ((s32)sBlueRobin.user_weight * 2205) / 1000; // Convert kg to lb

	// Init value index
	select = 0;	
		
	// Loop values until all are set or user breaks	set
	while(1) 
	{
		// Idle timeout : exit without saving 
		if (sys.flag.idle_timeout) break;

		// Button STAR (short): save, then exit 
		if (button.flag.star) 
		{
			// Store local variables in global structure
			sBlueRobin.calories 	= kcalories*1000;
			sBlueRobin.user_sex 	= sex;
			if (sys.flag.use_metric_units)	sBlueRobin.user_weight = weight;
			else							sBlueRobin.user_weight = (weight * 1000) / 2205;
			
			// Set display update flag
			display.flag.line1_full_update = 1;

			break;
		}

		switch (select)
		{
			case 0:		// Set calories
						display_symbol(LCD_UNIT_L2_KCAL, SEG_ON);
						set_value(&kcalories, 6, 5, 0, 199999, SETVALUE_DISPLAY_VALUE + SETVALUE_FAST_MODE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_5_0, display_value1);
						display_symbol(LCD_UNIT_L2_KCAL, SEG_OFF);
						clear_line(LINE2);
						select = 1;
						break;
			case 1:		// Set user sex
						set_value(&sex, 1, 0, 0, 1, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_SELECTION + SETVALUE_NEXT_VALUE, LCD_SEG_L2_3_0, display_selection_User_Sex1);
						select = 2;
						break;
			case 2:		// Set user weight
						if (sys.flag.use_metric_units)
						{
							display_chars(LCD_SEG_L2_1_0, (u8 *)"KG", SEG_ON);
							set_value(&weight, 3, 2, USER_WEIGHT_MIN_KG, USER_WEIGHT_MAX_KG, SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_4_2, display_value1);
						}
						else
						{
							display_chars(LCD_SEG_L2_1_0, (u8 *)"LB", SEG_ON);
							set_value(&weight, 3, 2, USER_WEIGHT_MIN_LB, USER_WEIGHT_MAX_LB, SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_4_2, display_value1);
						}
						select = 0;
						break;
		}
	}	

	// Clear button flags
	button.all_flags = 0;
}



// *************************************************************************************************
// @fn          sx_caldist
// @brief       Button DOWN toggles between calories and distance display.
// @param       u8 line		LINE2
// @return      none
// *************************************************************************************************
void sx_caldist(u8 line)
{
	// Clean up line
	display_caldist(line, DISPLAY_LINE_CLEAR);
	
	// Toggle display 
	if (sBlueRobin.caldist_view == 0) 	sBlueRobin.caldist_view = 1;
	else								sBlueRobin.caldist_view = 0;
	
	// Draw line
	display_caldist(line, DISPLAY_LINE_UPDATE_FULL);
}


// *************************************************************************************************
// @fn          display_heartrate
// @brief       Heart rate display routine. 
// @param       u8 line	LINE1
//				u8 update	DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_heartrate(u8 line, u8 update)
{
	u8 * str;
	
	if (update != DISPLAY_LINE_CLEAR)
	{
		if (is_bluerobin())
		{
			str = itoa(sBlueRobin.heartrate, 3, 2);
			display_chars(LCD_SEG_L1_2_0, str, SEG_ON);
		}
		else
		{
			display_chars(LCD_SEG_L1_2_0, (u8 *)"---", SEG_ON);
		}
	}
	
	// Redraw whole screen
	if (!is_bluerobin())
	{
		if (update == DISPLAY_LINE_UPDATE_FULL)	
		{
			 display_symbol(LCD_ICON_HEART, SEG_ON);
		}
		else if (update == DISPLAY_LINE_CLEAR)
		{
			// Clear heart when not connected
			display_symbol(LCD_ICON_HEART, SEG_OFF);
		}
	}
}


// *************************************************************************************************
// @fn          display_speed_kmh
// @brief       Speed display routine. Supports kmh and mph.
// @param       u8 line	LINE1
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_speed(u8 line, u8 update)
{
	u8 milesPerHour;
	u8 * str;

	// Speed resolution is 0.1 km/h 
	// Valid range is 0.0 .. 25.0 km/h 

	// Display resolution is 0.1km/h
	// For speed less than 1 km/h, force "0.x" display
	if (update != DISPLAY_LINE_CLEAR) 
	{
		if (sys.flag.use_metric_units)
		{
			str = itoa(sBlueRobin.speed, 3, 1);
		}
		else
		{
			milesPerHour = (u16)(sBlueRobin.speed * 0.6214);
			str = itoa(milesPerHour, 3, 1);
		}
		display_chars(LCD_SEG_L1_2_0, str, SEG_ON);
	}
		
	// Redraw whole screen
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		display_symbol(LCD_SEG_L1_DP0, SEG_ON);
		if (sys.flag.use_metric_units)
		{
			display_symbol(LCD_UNIT_L1_K, SEG_ON);
			display_symbol(LCD_UNIT_L1_M, SEG_ON);
		}
		else
		{
			display_symbol(LCD_UNIT_L1_M, SEG_ON);
			display_symbol(LCD_UNIT_L1_I, SEG_ON);
		}
		display_symbol(LCD_UNIT_L1_PER_H, SEG_ON);
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		display_symbol(LCD_SEG_L1_DP0, SEG_OFF);
		display_symbol(LCD_UNIT_L1_K, SEG_OFF);
		display_symbol(LCD_UNIT_L1_M, SEG_OFF);
		display_symbol(LCD_UNIT_L1_M, SEG_OFF);
		display_symbol(LCD_UNIT_L1_I, SEG_OFF);
		display_symbol(LCD_UNIT_L1_PER_H, SEG_OFF);
	}
}


// *************************************************************************************************
// @fn          display_distance
// @brief       Distance display routine. Supports km and mi.
// @param       u8 line		LINE2
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_distance(u8 line, u8 update)
{
	u8 * str;
	u32 miles;

	if (update != DISPLAY_LINE_CLEAR)
	{
		if (sys.flag.use_metric_units)
		{
			// Display distance in x.xx km format (resolution is 10m) up to 2000.00 km
			if (sBlueRobin.distance < 2000000) 
			{
				// Convert decimal distance in meters to format "xxxx.xx" km
				// If distance is less than 1000m, force display to "   0.xx"
				// If distance is less than 100m, force display to "   0.0x"
				str = itoa(sBlueRobin.distance/10, 6, 3);
			}
			else
			{
				str = itoa(199999, 6, 3);
			}	
		}
		else
		{
			// Convert km to miles, take care for "0.xx mi" display
			miles = (u32)(sBlueRobin.distance * 0.06214);
		
			// Display distance in x.xx mi format (resolution is 1/100mi) up to 2000.00 mi
			if (miles < 2000000) 
			{
				// If distance is less than 1 mile, force display to "   0.xx"
				// If distance is less than 1/10 mile, force display to "   0.0x"
				str = itoa(miles, 6, 3);
			}
			else
			{
				// Display maximum value (1999.99 mi)
				str = itoa(199999, 6, 3);
			}	
		}
		display_chars(LCD_SEG_L2_5_0, str, SEG_ON);
	}
	
	// Redraw whole screen
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		if (sys.flag.use_metric_units)
		{
			display_symbol(LCD_UNIT_L2_KM, SEG_ON);
		}
		else
		{
			display_symbol(LCD_UNIT_L2_MI, SEG_ON);
		}
		display_symbol(LCD_SEG_L2_DP, SEG_ON);
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		display_symbol(LCD_UNIT_L2_KM, SEG_OFF);
		display_symbol(LCD_UNIT_L2_MI, SEG_OFF);
		display_symbol(LCD_SEG_L2_DP, SEG_OFF);
	}	
}


// *************************************************************************************************
// @fn          display_caldist
// @brief       Shared calories/distance display routine. 
// @param       u8 line	LINE2
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_caldist(u8 line, u8 update)
{
	if (sBlueRobin.caldist_view == 0) 	display_calories(line, update);
	else								display_distance(line, update);
}


// *************************************************************************************************
// @fn          display_calories
// @brief       Calories display routine. 
// @param       u8 line	LINE2
//				u8 update	DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_calories(u8 line, u8 update)
{
	u8 * str;
		
	if (update != DISPLAY_LINE_CLEAR)
	{
		// Convert decimal calories to string
		str = itoa(sBlueRobin.calories / 1000, 6, 5);
		display_chars(LCD_SEG_L2_5_0, str, SEG_ON);
	}
	
	// Redraw whole screen
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		display_symbol(LCD_UNIT_L2_KCAL, SEG_ON);
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		// Clean up symbols when leaving function
		display_symbol(LCD_UNIT_L2_KCAL, SEG_OFF);
	}
}


// *************************************************************************************************
// @fn          is_bluerobin
// @brief       Returns TRUE if BlueRobin transmitter is connected. 
// @param       none
// @return      u8
// *************************************************************************************************
u8 is_bluerobin(void)
{
	return (sBlueRobin.state == BLUEROBIN_CONNECTED);
}


// *************************************************************************************************
// @fn          is_bluerobin_searching
// @brief       Returns TRUE if BlueRobin is searching for a transmitter. 
// @param       none
// @return      u8
// *************************************************************************************************
u8 is_bluerobin_searching(void)
{
	return (sBlueRobin.state == BLUEROBIN_SEARCHING);
}


// *************************************************************************************************
// @fn          get_bluerobin_data
// @brief       Read BlueRobin packet data from API. 
// @param       none
// @return      none
// *************************************************************************************************
void get_bluerobin_data(void)
{
	u16 calories;
	brtx_state_t bChannelState;
	
	// Check connection status
	bChannelState = BRRX_GetState_t(HR_CHANNEL);
	
	switch (bChannelState)
	{
		case TX_ACTIVE: 	// Read heart rate data from BlueRobin API
							sBlueRobin.heartrate = BRRX_GetHeartRate_u8();
							
							// Read speed from BlueRobin API (only valid if sender is USB dongle)
							sBlueRobin.speed 	  = BRRX_GetSpeed_u8();
							
							// Read distance from BlueRobin API (only valid if sender is USB dongle)
							sBlueRobin.distance  = BRRX_GetDistance_u16();
							if (sBlueRobin.distance > 2000000) sBlueRobin.distance = 0;
							
							// Heart rate high enough for calorie measurement?
							if (sBlueRobin.heartrate >= 65 && sBlueRobin.user_weight != 0) 
							{
								calories = ((sBlueRobin.heartrate - 60) * sBlueRobin.user_weight) / 32;
								
								// Calorie reduction for female user required?
								if (sBlueRobin.user_sex == USER_SEX_FEMALE)
								{ 
									calories -= calories / 4;
								}
								
								// Restart from 0 when reaching 199999 kcal
								sBlueRobin.calories += calories;
								if (sBlueRobin.calories > 200000000) sBlueRobin.calories = 0;
							}
							sBlueRobin.update = BLUEROBIN_NEW_DATA;
							break;

		case TX_OFF:		// Shutdown connection
						    stop_bluerobin();
							break;

		// BR_SEARCH, BR_LEARN, BR_PAUSE: Keep old values until we receive new data
		default:			break;	
	}
}



// *************************************************************************************************
// @fn          stop_bluerobin
// @brief       Stop communication and put peripherals in power-down mode.
// @param       none
// @return      none
// *************************************************************************************************
void stop_bluerobin(void)
{
	// Reset connection status byte	
	sBlueRobin.state = BLUEROBIN_OFF;	

	// Stop channel
	BRRX_Stop_v(HR_CHANNEL);

	// Powerdown radio
	close_radio();
	
	// Force full display update to clear heart rate and speed data
	sBlueRobin.heartrate 		= 0;
	sBlueRobin.speed 			= 0;
	sBlueRobin.distance 		= 0;
	display.flag.full_update 	= 1;
	
	// Clear heart and RF symbol 
	display_symbol(LCD_ICON_HEART, SEG_OFF_BLINK_OFF);	
	display_symbol(LCD_ICON_BEEPER1, SEG_OFF_BLINK_OFF);
	display_symbol(LCD_ICON_BEEPER2, SEG_OFF_BLINK_OFF);
	display_symbol(LCD_ICON_BEEPER3, SEG_OFF_BLINK_OFF);
}

