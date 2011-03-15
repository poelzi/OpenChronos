// *************************************************************************************************
//
//	Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/ 
//	Copyright (C) 2011 Frank Van Hooft
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
// Altitude measurement functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

#ifdef FEATURE_ALTITUDE

// driver
#include "altitude.h"
//#ifdef CONFIG_ALTI_ACCUMULATOR
//#include "alt_accu.h"
//#endif
#include "display.h"
#include "vti_ps.h"
#include "ports.h"
#include "timer.h"

// logic
#include "user.h"
#ifdef CONFIG_VARIO
# include "vario.h"
#endif


// *************************************************************************************************
// Prototypes section


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section
struct alt sAlt;

#ifdef CONFIG_ALTI_ACCUMULATOR

#define	ALT_ACCUM_DIR_THRESHOLD  5 // change in meters needed to switch direction up <-> down

// The following used by the altitude accumulation function
u8  alt_accum_enable;		// 1 means the altitude accumulator is enabled, zero means disabled
s32 alt_accum_startpoint;	// altitude in metres that user "zeroed" the accumulator at
s32 alt_accum__accumtotal;	// total accumulated upwards vertical in metres
s32 alt_accum_lastpeakdip;	// altitude in metres of last "inflection point", either a peak or a dip
u8  alt_accum_direction;	// 1 means we're currently going up (last peakdip was a dip), 0 means we're going down
s32 alt_accum_prevalt;		// previous altitude - altitude the last time we read the altimeter 
s32 alt_accum_max;		// maximum altitude encountered
u8  alt_accum_displaycode;	// what to display
#endif

// *************************************************************************************************
// Extern section

// Global flag for pressure sensor initialisation status
extern u8 ps_ok;


// *************************************************************************************************
// @fn          reset_altitude_measurement
// @brief       Reset altitude measurement.
// @param       none
// @return      none
// *************************************************************************************************
void reset_altitude_measurement(void)
{
	// Menu item is not visible
	sAlt.state 		= MENU_ITEM_NOT_VISIBLE;

	// Clear timeout counter
	sAlt.timeout	= 0;
	
	// Set default altitude value
	sAlt.altitude		= 0;
	
	// Pressure sensor ok?
	if (ps_ok)
	{
		// Initialise pressure table
		init_pressure_table();
		
		// Do single conversion
		start_altitude_measurement();
		stop_altitude_measurement();	

		// Apply calibration offset and recalculate pressure table
		if (sAlt.altitude_offset != 0)
		{
			sAlt.altitude += sAlt.altitude_offset;
			update_pressure_table(sAlt.altitude, sAlt.pressure, sAlt.temperature);
		}
	}
}

#ifndef CONFIG_METRIC_ONLY
// *************************************************************************************************
// @fn          conv_m_to_ft
// @brief       Convert meters to feet
// @param       u16 m		Meters
// @return      u16		Feet
// *************************************************************************************************
s16 convert_m_to_ft(s16 m)
{
	return (((s32)328*m)/100);
}


// *************************************************************************************************
// @fn          conv_ft_to_m
// @brief       Convert feet to meters
// @param       u16 ft		Feet
// @return      u16		Meters
// *************************************************************************************************
s16 convert_ft_to_m(s16 ft)
{
	return (((s32)ft*61)/200);
}

#endif

// *************************************************************************************************
// @fn          is_altitude_measurement
// @brief       Altitude measurement check
// @param       none
// @return      u8		1=Measurement ongoing, 0=measurement off
// *************************************************************************************************
u8 is_altitude_measurement(void)
{
	return ((sAlt.state == MENU_ITEM_VISIBLE) && (sAlt.timeout > 0));
}


// *************************************************************************************************
// @fn          start_altitude_measurement
// @brief       Start altitude measurement
// @param       none
// @return      none
// *************************************************************************************************
void start_altitude_measurement(void)
{
	// Show warning if pressure sensor was not initialised properly
	if (!ps_ok) 
	{
		display_chars(LCD_SEG_L1_2_0, (u8*)"ERR", SEG_ON);
		return;
	}

	// Start altitude measurement if timeout has elapsed
	if (sAlt.timeout == 0)
	{
		// Enable DRDY IRQ on rising edge
		PS_INT_IFG &= ~PS_INT_PIN;
		PS_INT_IE |= PS_INT_PIN;

		// Start pressure sensor
		ps_start(); 

		// Set timeout counter only if sensor status was OK
		sAlt.timeout = ALTITUDE_MEASUREMENT_TIMEOUT;

		// Get updated altitude
		while((PS_INT_IN & PS_INT_PIN) == 0); 
		do_altitude_measurement(FILTER_OFF);
	}
}


// *************************************************************************************************
// @fn          stop_altitude_measurement
// @brief       Stop altitude measurement
// @param       none
// @return      none
// *************************************************************************************************
void stop_altitude_measurement(void)
{
	// Return if pressure sensor was not initialised properly
	if (!ps_ok) return;
	
	// Stop pressure sensor
	ps_stop();
	
	// Disable DRDY IRQ
	PS_INT_IE  &= ~PS_INT_PIN;
	PS_INT_IFG &= ~PS_INT_PIN;
	
	// Clear timeout counter
	sAlt.timeout = 0;
}



// *************************************************************************************************
// @fn          do_altitude_measurement
// @brief       Perform single altitude measurement
// @param       none
// @return      none
// *************************************************************************************************
void do_altitude_measurement(u8 filter)
{
	volatile u32 pressure;

	// If sensor is not ready, skip data read	
	if ((PS_INT_IN & PS_INT_PIN) == 0) return;
		
	// Get temperature (format is *10?K) from sensor
	sAlt.temperature = ps_get_temp();

	// Get pressure (format is 1Pa) from sensor
	pressure = ps_get_pa();	
		
	// Store measured pressure value
	if (filter == FILTER_OFF) //sAlt.pressure == 0) 
	{
		sAlt.pressure = pressure;
	}
	else
	{
		// Filter current pressure
#ifdef FIXEDPOINT
        pressure = (u32)(((pressure * 2) + (sAlt.pressure * 8))/10);
#else
		pressure = (u32)((pressure * 0.2) + (sAlt.pressure * 0.8));
#endif
		// Store average pressure
		sAlt.pressure = pressure;
	}

	// Convert pressure (Pa) and temperature (?K) to altitude (m).
#ifdef FIXEDPOINT
	sAlt.altitude = conv_pa_to_altitude(sAlt.pressure, sAlt.temperature);
#else
    sAlt.altitude = conv_pa_to_meter(sAlt.pressure, sAlt.temperature);
#endif

#ifdef CONFIG_VARIO
   // Stash a copy to the vario after filtering. If doing so before, there
   // is just too much unnecessary fluctuation, up to +/- 7Pa seen.
   vario_p_write( pressure );
#endif
}


// *************************************************************************************************
// @fn          sx_altitude
// @brief       Altitude direct function. Sx restarts altitude measurement.
// @param       u8 line	LINE1, LINE2
// @return      none
// *************************************************************************************************
void sx_altitude(u8 line)
{
	// Function can be empty
	
	// Restarting of altitude measurement will be done by subsequent full display update 
}


// *************************************************************************************************
// @fn          mx_altitude
// @brief       Mx button handler to set the altitude offset. 
// @param       u8 line		LINE1
// @return      none
// *************************************************************************************************
void mx_altitude(u8 line)
{
	s32 altitude;
	s32	limit_high, limit_low;

	// Clear display
	clear_display_all();
#ifdef CONFIG_ALTI_ACCUMULATOR
	// Display "ALt" on top line
	display_chars(LCD_SEG_L1_3_0, (u8*)"ALT ", SEG_ON);
	clear_line(LINE2);
#endif

	// Set lower and upper limits for offset correction
#ifdef CONFIG_METRIC_ONLY
	display_symbol(LCD_UNIT_L1_M, SEG_ON);

	// Convert global variable to local variable
	altitude  = sAlt.altitude; 

	// Limits for set_value function
	limit_low = -100;
	limit_high = 9000;
#else
	if (sys.flag.use_metric_units)
	{
		// Display "m" symbol
		display_symbol(LCD_UNIT_L1_M, SEG_ON);

		// Convert global variable to local variable
		altitude  = sAlt.altitude; 

		// Limits for set_value function
		limit_low = -100;
		limit_high = 9000;
	}
	else // English units
	{
		// Display "ft" symbol
		display_symbol(LCD_UNIT_L1_FT, SEG_ON);
		
		// Convert altitude in meters to feet
		altitude = sAlt.altitude;

 		// Convert from meters to feet
		altitude = convert_m_to_ft(altitude);		

		// Limits for set_value function
		limit_low = -500;
#ifdef CONFIG_ALTI_ACCUMULATOR
		limit_high = 33000;
#else
		limit_high = 9999;
#endif
	}
#endif
	// Loop values until all are set or user breaks	set
	while(1) 
	{
		// Idle timeout: exit without saving 
		if (sys.flag.idle_timeout) break;

		// Button STAR (short): save, then exit 
		if (button.flag.star) 
		{
			// When using English units, convert ft back to m before updating pressure table
#ifndef CONFIG_METRIC_ONLY
			if (!sys.flag.use_metric_units) altitude = convert_ft_to_m((s16)altitude);
#endif

			// Update pressure table
			update_pressure_table((s16)altitude, sAlt.pressure, sAlt.temperature);
			
			// Set display update flag
			display.flag.line1_full_update = 1;

			break;
		}

		// Set current altitude - offset is set when leaving function
#ifdef CONFIG_ALTI_ACCUMULATOR
		// use 2nd line as it displays larger numbers
		set_value(&altitude, 5, 4, limit_low, limit_high, SETVALUE_DISPLAY_VALUE + SETVALUE_FAST_MODE + SETVALUE_DISPLAY_ARROWS, LCD_SEG_L2_4_0, display_value1);
#else
		set_value(&altitude, 4, 3, limit_low, limit_high, SETVALUE_DISPLAY_VALUE + SETVALUE_FAST_MODE + SETVALUE_DISPLAY_ARROWS, LCD_SEG_L1_3_0, display_value1);
#endif
	}		
	
	// Clear button flags
	button.all_flags = 0;
}


// *************************************************************************************************
// @fn          display_altitude
// @brief       Display routine. Supports display in meters and feet. 
// @param       u8 line			LINE1
//				u8 update		DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
#ifdef CONFIG_ALTITUDE
void display_altitude(u8 line, u8 update)
{
	u8 * str;
#ifndef CONFIG_METRIC_ONLY
	s16 ft;
#endif
	
	// redraw whole screen
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		// Enable pressure measurement
		sAlt.state = MENU_ITEM_VISIBLE;

		// Start measurement
		start_altitude_measurement();
#ifdef CONFIG_ALTI_ACCUMULATOR
		display_chars(LCD_SEG_L1_3_0, (u8*)"ALT ", SEG_ON);
#endif
#ifdef CONFIG_METRIC_ONLY
			display_symbol(LCD_UNIT_L1_M, SEG_ON);
#else		
		if (sys.flag.use_metric_units)
		{
			// Display "m" symbol
			display_symbol(LCD_UNIT_L1_M, SEG_ON);
		}
		else
		{
			// Display "ft" symbol
			display_symbol(LCD_UNIT_L1_FT, SEG_ON);
		}
#endif		
		// Display altitude
		display_altitude(LINE1, DISPLAY_LINE_UPDATE_PARTIAL);
	}
	else if (update == DISPLAY_LINE_UPDATE_PARTIAL)
	{
		// Update display only while measurement is active
		if (sAlt.timeout > 0)
		{
#ifndef CONFIG_METRIC_ONLY
			if (sys.flag.use_metric_units)
			{
#endif
				// Display altitude in xxxx m format, allow 3 leading blank digits
				if (sAlt.altitude >= 0)
				{
#ifdef CONFIG_ALTI_ACCUMULATOR
					str = itoa(sAlt.altitude, 5, 4);
#else
					str = itoa(sAlt.altitude, 4, 3);
#endif
					display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
				}
				else
				{
#ifdef CONFIG_ALTI_ACCUMULATOR
					str = itoa(sAlt.altitude*(-1), 4, 3);
#else
					str = itoa(sAlt.altitude*(-1), 5, 4);
#endif
					display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
				}
#ifndef CONFIG_METRIC_ONLY
			}
			else
			{
				// Convert from meters to feet
				ft = convert_m_to_ft(sAlt.altitude);
#ifndef CONFIG_ALTI_ACCUMULATOR
				// Limit to 9999ft (3047m)
				if (ft > 9999) ft = 9999;
#endif
				// Display altitude in xxxx ft format, allow 3 leading blank digits
				if (ft >= 0)
				{
#ifdef CONFIG_ALTI_ACCUMULATOR
					str = itoa(ft, 4, 3);
#else
					str = itoa(ft, 5, 4);
#endif
					display_symbol(LCD_SYMB_ARROW_UP, SEG_ON);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
				}
				else
				{
#ifdef CONFIG_ALTI_ACCUMULATOR
					str = itoa(ft*(-1), 4, 3);
#else
					str = itoa(ft*(-1), 5, 4);
#endif
					display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
					display_symbol(LCD_SYMB_ARROW_DOWN, SEG_ON);
				}				
			}
#endif
#ifdef CONFIG_ALTI_ACCUMULATOR
			// display altitude on bottom line (5 digits)
			clear_line(LINE2);
			display_chars(LCD_SEG_L2_4_0, str, SEG_ON);
#else
			display_chars(LCD_SEG_L1_3_0, str, SEG_ON);
#endif
		}
	}
	else if (update == DISPLAY_LINE_CLEAR)
	{
		// Disable pressure measurement
		sAlt.state = MENU_ITEM_NOT_VISIBLE;

		// Stop measurement
		stop_altitude_measurement();
		
		// Clean up function-specific segments before leaving function
#ifdef CONFIG_ALTI_ACCUMULATOR
		// clear off the altitude display from the second line
		clear_line(LINE2);
		// should really try to get the date displayed here again too
#endif
		display_symbol(LCD_UNIT_L1_M, SEG_OFF);
		display_symbol(LCD_UNIT_L1_FT, SEG_OFF);
		display_symbol(LCD_SYMB_ARROW_UP, SEG_OFF);
		display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
	}
}

#ifdef CONFIG_ALTI_ACCUMULATOR
// *************************************************************************************************
// @fn          altitude_accumulator_periodic
// @brief       Is called periodically, reads altitude and accumulates upwards vertical
// @param       none
// @return      none
//
// This function is called once a minute. It reads the current altitude, then uses that to accumulate
// upwards altitude only (it does not measure or accumulate downwards altitude - only altitude gains.
// It functions as follows. Current direction (either up or down) is given in alt_accum_direction.
//
// If we're currently going up, then alt_accum_lastpeakdip contains the altitude of the last dip (valley)
// we encountered and we're rising up above that. If the current altimeter value is greater than the
// previous altimeter reading alt_accum_prevalt, then update alt_accum_prevalt with the current altitude
// and we're done. If however we've dropped below the previous value, then if we've exceeded our
// "direction change threshold" ALT_ACCUM_DIR_THRESHOLD, then it appears we've peaked and have started
// heading downhill. So add (alt_accum_prevalt - alt_accum_lastpeakdip) to alt_accum__accumtotal to
// collect that recently-finished uphill into our accumulated uphill total. Then reverse course:
// set alt_accum_lastpeakdip equal to alt_accum_prevalt so that lastpeakdip contains the altitude of the
// top of that recently-crossed hill (it's now a peak altitude rather than a dip (valley) altitude),
// change the alt_accum_direction flag to downhill, and as usual set the alt_accum_prevalt to our current
// altitude.
//
// On the other hand, if our current altitude is only slightly less than the previous altitude, then
// do nothing - not even update our "previous" altitude. When we're going uphill, only 2 things matter to
// us: either we've gone uphill some more, or we've gone downhill enough to trigger the change-or-direction
// threshold.
//
// If we're going downhill (direction flag alt_accum_direction says downhill), there's little to do except
// keep updating alt_accum_prevalt with the current altitude if we've dropped lower, and watch for a
// change of direction to an uphill. Much the same as the uphill case, but we don't accumulate downhills
// once the change of direction occurs.
//
// In addition, independently of all this, we also update alt_accum_max with the highest altitude
// we've found.
//
// All measurements are recorded & stored in metres.
//
// Yes it's a bit convoluted. That's OK. The basic thing to understand is that a "simple" accumulator
// simply adds altitude every time we take an altimeter reading. However every reading has an error, so
// adding a bunch of readings results in a lot of error. For that reason we have this more complicated
// system whereby we look for peaks and dips, and only add to the accumulator when we find the next peak.
// In that way we obtain the best accumulator accuracy possible. It does make for a more complicated
// system though.
//
// *************************************************************************************************
void altitude_accumulator_periodic (void)
{
	s32 currentalt;					// our current altitude

	// First a quick sanity check. If we're not supposed to be running, something's wrong, so just exit
	if (alt_accum_enable==0) return;

	// First thing we need to know is our current altitude. Take 4 measurements & average them.
	start_altitude_measurement();
	stop_altitude_measurement();
	currentalt = sAlt.altitude;			// first reading

	// Now it's comparisions time. First we'll quickly update the maximum altitude tracker
	if (currentalt > alt_accum_max)
		alt_accum_max = currentalt;		// update max altitude if we're at a new high

	// Now our convoluted altitude accumulator, looking for peaks and valleys, etc.
	if (alt_accum_direction) {
		// Execute here if we're supposedly going upwards
		if (currentalt >= alt_accum_prevalt) {
			// Execute here if we're still going upwards - current alt is greater than previous alt
			alt_accum_prevalt = currentalt;	// just update our "previous" value for next time
			return;								// and that's it - we're done
		}
		else {
			// Execute here if our current altitude is below our previous - have we crested the hill and
			// started to descend? If we've exceeded the threshold altitude drop we need to deal with that.
			if ((alt_accum_prevalt - currentalt) >= ALT_ACCUM_DIR_THRESHOLD) {
				// Execute here if we've descended enough off the hillcrest to exceed the threshold - we've just
				// gone through a change of direction, so we need to accumulate the previously gained altitude,
				// then set things up for going downhill now.
				alt_accum__accumtotal += alt_accum_prevalt - alt_accum_lastpeakdip;	// accumulate the vertical from that last hill climb
				alt_accum_lastpeakdip = alt_accum_prevalt;				// peakdip is now a peak elevation
				alt_accum_direction = 0;						// indicate we're tracking downhill now
				return;
			}
			else	// we've dropped a little, but not enough to trigger any action yet
				return;
		}
	}
	else {
		// Execute here if we're supposedly going downwards
		if (currentalt <= alt_accum_prevalt) {
			// Execute here if we're still going downwards - current alt is less than previous alt
			alt_accum_prevalt = currentalt;		// just update our "previous" value for next time
			return;					// and that's it - we're done
		}
		else {
			// Execute here if our current altitude is above our previous - have we bottomed the valley and
			// started to ascend? If we've exceeded the threshold altitude increase we need to deal with that.
			if ((currentalt - alt_accum_prevalt) >= ALT_ACCUM_DIR_THRESHOLD) {
				// Execute here if we've ascended enough above the valley floor to exceed the threshold - we've just
				// gone through a change of direction, so we need to set things up for going uphill now.
				alt_accum_lastpeakdip = alt_accum_prevalt;	// peakdip is now a dip (valley) elevation
				alt_accum_direction = 1;			// indicate we're tracking uphill now
				return;
			}
			else	// we've ascended a little, but not enough to trigger any action yet
				return;
		}
	}
}


// *************************************************************************************************
// @fn          altitude_accumulator_start
// @brief       Initialises the altitude accumulator function
// @param       none
// @return      none
// *************************************************************************************************
void altitude_accumulator_start (void)
{
	s32 temp;

	alt_accum__accumtotal = 0;		// So far total upwards vertical accumulation is zero
	alt_accum_direction = 1;		// start off by assuming we're heading uphill

	// Now let's get 4 altitude readings, then average them, to obtain our current altitude
	start_altitude_measurement();
	stop_altitude_measurement();
	temp = sAlt.altitude;				// first reading
	/* start_altitude_measurement();
	   stop_altitude_measurement();
	   temp += sAlt.altitude;			// second reading
	   start_altitude_measurement();
	   stop_altitude_measurement();
	   temp += sAlt.altitude;			// third reading
	   start_altitude_measurement();
	   stop_altitude_measurement();
	   temp += sAlt.altitude;			// fourth reading
	   temp = temp >> 2;				// divide result by 4 = our current altitude */

	alt_accum_startpoint = temp;		// the altitude the user zeroed the accumulator at
	alt_accum_lastpeakdip = temp;		// altitude of the last dip (in this case, as we assume we're going uphill)
	alt_accum_prevalt = temp;		// previous altitude value
	alt_accum_max = temp;			// maximum altitude we've encountered so far
}



// *************************************************************************************************
// @fn          display_selection_altaccum
// @brief       Display altitude accumulator ON or OFF
// @param       u8 segments			where to display, usually LCD_SEG_L2_4_0
//				u32 index			0 = OFF, 1 = ON
//				u8 digits			Not used
//				u8 blanks			Not used
// @return      none
// *************************************************************************************************
void display_selection_altaccum (u8 segments, u32 index, u8 digits, u8 blanks)
{
	if (index) {
		clear_line(LINE2);
		display_chars(segments, (u8*)" ON  ", SEG_ON_BLINK_ON);		// display "ON" on bottom line
	}
	else {
		clear_line(LINE2);
		display_chars(segments, (u8*)" OFF ", SEG_ON_BLINK_ON);		// display "OFF" on bottom line
	}
}




// *************************************************************************************************
// @fn          sx_alt_accumulator
// @brief       Altitude accumulator direct function. Called when "up" button is pressed.
// @param       u8 line	LINE1, LINE2
// @return      none
//
// Simply increments alt_accum_displaycode so that the display function display_alt_accumulator()
// knows what to display on the bottom line. Codes are:
// alt_accum_displaycode = 0:  Altitude relative to start point
// alt_accum_displaycode = 1:  Total accumulated upwards vertical altitude
// alt_accum_displaycode = 2:  Maximum altitude encountered (max height)
// *************************************************************************************************
void sx_alt_accumulator(u8 line)
{
	alt_accum_displaycode++;

	if (alt_accum_displaycode > 2)
		alt_accum_displaycode = 0;
}


// *************************************************************************************************
// @fn          display_alt_accumulator
// @brief       Display altitude accumulator routine. Supports display in meters and feet.
// @param       u8 line			LINE1
//				u8 update		DISPLAY_LINE_UPDATE_FULL, DISPLAY_LINE_UPDATE_PARTIAL, DISPLAY_LINE_CLEAR
// @return      none
// *************************************************************************************************
void display_alt_accumulator (u8 line, u8 update)
{
	s32 temp;
	u8 * str;


	// show our altitude accumulator numbers on the second line
	if ( (update==DISPLAY_LINE_UPDATE_FULL) || (update==DISPLAY_LINE_UPDATE_PARTIAL) )
	{
		// Show "ALtA" on top line
		display_chars(LCD_SEG_L1_3_0, (u8*)"ALTA", SEG_ON);

		// if the altitude accumulator is currently disabled, we've got nothing else to display so exit
		if (alt_accum_enable == 0) {
			clear_line(LINE2);
			display_chars(LCD_SEG_L2_4_0, (u8*)" OFF ", SEG_ON);	// display "OFF" on bottom line
			return;
		}

		// Otherwise the accumulator is running, so display on the second line whatever alt_accum_displaycode
		// says to display, in metres or feet as appropriate.
		if (alt_accum_displaycode>2) alt_accum_displaycode=0;		// sanity check

		// light up "m" or "ft" display symbol as appropriate
		if (sys.flag.use_metric_units)
			display_symbol(LCD_UNIT_L1_M, SEG_ON);			// metres symbol
		else
			display_symbol(LCD_UNIT_L1_FT, SEG_ON);			// or feet symbol

		if (alt_accum_displaycode==0)
		{
			// Display current altitude relative to the accumulator's starting point
			// "DIFF" means difference between starting elevation & current elevation
			display_chars(LCD_SEG_L1_3_0, (u8*)"DIFF", SEG_ON);		// top line display message

			start_altitude_measurement();
			stop_altitude_measurement();					// grab our current altitude

			temp = sAlt.altitude - alt_accum_startpoint;	// difference between starting altitude & current altitude
			if (sys.flag.use_metric_units==0) temp = (temp*328)/100;	// convert to feet if necessary

			clear_line(LINE2);						// clear the bottom line of the display
			if (temp < 0) {							// if altitude is a negative number...
				display_char(LCD_SEG_L2_4, '-', SEG_ON);		// display - (negative sign) character at start of second line
				temp = 0 - temp;					// make altitude a positive number again so we can display it
				if (temp>9999) temp = 9999;				// we can only display 4 digits for a negative number
				str = itoa(temp, 4, 3);					// 4 digits, up to 3 leading blank digits
				display_chars(LCD_SEG_L2_3_0, str, SEG_ON);		// display altitude difference on bottom line (4 digits)
				return;
			}
			else								// otherwise altitude difference is a positive number
			{
				str = itoa(temp, 5, 4);					// 5 digits, up to 4 leading blank digits
				display_chars(LCD_SEG_L2_4_0, str, SEG_ON);		// display altitude difference on bottom line (5 digits)
				return;
			}
		}

		else if (alt_accum_displaycode==1)
		{
			// Display total accumulated elevation gain. Remember we might currently be going uphill
			// so we need to check for, and include, any current elevation gain
			display_chars(LCD_SEG_L1_3_0, (u8*)"ACCA", SEG_ON);		// top line display message
			clear_line(LINE2);						// clear the bottom line of the display

			if (alt_accum__accumtotal<0) alt_accum__accumtotal = 0;	// accumulated total should never be negative!
			temp = alt_accum__accumtotal;				// local copy of accumulated total

			// Now we need to add on any vertical gained recently, that hasn't yet been included in alt_accum__accumtotal
			// This only happens if we're currently going uphill, and we've above our last valley / dip elevation
			if (alt_accum_direction && (sAlt.altitude>alt_accum_lastpeakdip))	// if we're going up, and we're higher than our last dip (valley) altitude
				temp += sAlt.altitude - alt_accum_lastpeakdip;			// then add the vertical we've gained so far above that last dip / valley point

			// display the result
			str = itoa(temp, 5, 4);					// 5 digits, up to 4 leading blank digits
			display_chars(LCD_SEG_L2_4_0, str, SEG_ON);		// display peak altitude on bottom line (5 digits)
			return;
		}

		else
		{
			// Display maximum altitude found so far
			display_chars(LCD_SEG_L1_3_0, (u8*)"PEAK", SEG_ON);	// top line display message
			clear_line(LINE2);					// clear the bottom line of the display

			temp = alt_accum_max;					// local copy of peak altitude
			if (temp < 0) temp = 0;					// I can't be bothered displaying a negative number! So make it zero if it is.
			str = itoa(temp, 5, 4);					// 5 digits, up to 4 leading blank digits
			display_chars(LCD_SEG_L2_4_0, str, SEG_ON);		// display peak altitude on bottom line (5 digits)
			return;
		}
	}


	// clear out - we're finished
	else if (update == DISPLAY_LINE_CLEAR)
	{
		clear_line(LINE2);			// clear off the altitude display from the second line
		// should really try to get the date displayed here again

		// Clean up function-specific segments before leaving function
		display_symbol(LCD_UNIT_L1_M, SEG_OFF);
		display_symbol(LCD_UNIT_L1_FT, SEG_OFF);
		display_symbol(LCD_SYMB_ARROW_DOWN, SEG_OFF);
	}
}



// *************************************************************************************************
// @fn          mx_alt_accumulator
// @brief       Altitude accumulator modify function. Turn accumulator function on or off.
// @param       u8 line	LINE1, LINE2
// @return      none
// *************************************************************************************************
void mx_alt_accumulator(u8 line)
{
	s32 temp_enable;				// local copy of the global altitude accumumulator enable flag


	// Show "ALtA" on top line
	display_chars(LCD_SEG_L1_3_0, (u8*)"ALTA", SEG_ON);

	// Allow the user to turn the altitude accumulator function ON or OFF. This is stored in global variable
	// alt_accum_enable where 0 = off, 1 = on. Display "on" or "off" on the bottom line as appropriate.
	temp_enable = alt_accum_enable;			// local copy, to allow user to modify it
	set_value(&temp_enable, 1, 0, 0, 1, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_SELECTION, LCD_SEG_L2_4_0, display_selection_altaccum);	// allow user to turn on / off

	// If the altitude accumulator has just been enabled, call its initialisation routine
	if ( (temp_enable==1) && (alt_accum_enable==0) )
		altitude_accumulator_start();

	alt_accum_enable = temp_enable;		// global flag that the accumulator is running, or not, as the user selected

	clear_line(LINE2);			// don't display on/off on bottom line any more
	button.all_flags = 0;			// clear all pushbutton flags
}


#endif // CONFIG_ALTI_ACCUMULATOR

#endif // CONFIG_ALTITUDE

#endif // FEATURE_ALTITUDE
