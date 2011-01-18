/*
    Altivario function for ez430 chronos watch.

    Copyright (C) 2011, Marc Bongartz <mbong@free.fr>

    Build environment Copyright (C) 2010 Marc Poulhi<E8>s <dkm@kataplop.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#include "project.h"

#ifdef CONFIG_VARIO

// driver
#include "display.h"
#include "buzzer.h"

// logic
#include "vario.h"

//
// Module internal definitions.
//

//
// Global struct with all our variables.
//
struct
{
   u32 pressure;  // updated by altitude.c - need a mutex ?
   u32 prev_pa;   // pressure at last scan
   u8 p_valid;    // mutex for pressure field
   u8 view_mode;  // view mode, controlled by "v" key
   u8 beep_mode;  // beeper mode, controlled by "#" key
   struct
     {
	s16 vzmin; // Vz min in Pascal
	s16 vzmax; // Vz max in Pascal
     } stats;
} G_vario;

//
// Note the beepmode enum changes are reflected in the beepmode symbol.
// For visual feedback during settings, the beeper2 symbol is turned on
// when beepmode ASCENT_0 or BOTH are selected. Update the corresponding
// logic if you change the beepmode enum order or add a new enum value.
// 
enum
{
   VARIO_BEEPMODE_OFF = 0,    // default beep mode
   VARIO_BEEPMODE_ASCENT_0,   // ascent only, beep at zero
   VARIO_BEEPMODE_ASCENT_1,   // ascent only, beep at one
   VARIO_BEEPMODE_BOTH,       // ascent and descent
   VARIO_BEEPMODE_MAX
};

enum 
{
   VARIO_VIEWMODE_ALT_M = 0,  // Vario in m/s, default view mode
   VARIO_VIEWMODE_ALT_PA,     // Vario in Pascal
   VARIO_VIEWMODE_PA,         // Display current pressure
   VARIO_VIEWMODE_VZMAX,      // Max Vario (positive)
   VARIO_VIEWMODE_VZMIN,      // Max Vario (positive)
   VARIO_VIEWMODE_MAX
};

//
// Clear statistics
//
static inline void
_clear_stats( void )
{
   G_vario.stats.vzmin = 0;
   G_vario.stats.vzmax = 0;
}

// "v" button press changes display mode
extern void
sx_vario(u8 line)
{
   G_vario.view_mode++;
   G_vario.view_mode %= VARIO_VIEWMODE_MAX;
}

//
// Long "#" press will turn on/off vario sound if in vario display,
// or clear stats if in stats view mode.
// 
extern void
mx_vario(u8 line)
{
   switch( G_vario.view_mode )
     {
      case VARIO_VIEWMODE_ALT_M:
      case VARIO_VIEWMODE_ALT_PA:
	G_vario.beep_mode++;
	G_vario.beep_mode %= VARIO_BEEPMODE_MAX;
	break;
      
      case VARIO_VIEWMODE_VZMAX:
      case VARIO_VIEWMODE_VZMIN:
	_clear_stats();
	break;

      case VARIO_VIEWMODE_PA:  // no settings for pressure display
      case VARIO_VIEWMODE_MAX:
	break;
     }
}

//
// Mutex for accessing G_vario.pressure from altitude.c (write) and vario.c (read)
// This prevents badly timed updates, for example on button press. It also helped
// me figure a problem with calls to display_vario() from interrupt level.
//
extern void
vario_p_write( u32 p )
{
   if ( !G_vario.p_valid )
     {	
	G_vario.pressure = p;
	G_vario.p_valid++;
     }
   else
     {
	//
	// Pressure has not been read by the vario (called once per second)
	// since last write from altimeter (called about once every second in
	// ultra low power mode, more often for other modes, or on demand for
	// low power with external trigger mode), average in the new pressure
	// value.
	//
	G_vario.pressure = (G_vario.pressure + p) >> 1;
     }
}

int vario_p_read( u32 *retp )
{
   if ( G_vario.p_valid )
     {
	*retp = G_vario.pressure;
	G_vario.p_valid = 0;
	return 0; // success, a new pressure value is available.
     }
   return 1; // error, no new pressure value available.
}

//
// Produce a sound depending on the ascent/descent rate.
//
void chirp( s16 pdiff )
{
   const u8 center_steps = 8;
   const u8 range_steps  = 5;
   u8 bsteps;
   u8 nchirps;
   u8 on_time;

   //
   // Buzzer steps (see driver/buzzer) 3..23 provide a frequency
   // of 4096Hz..682Hz, well within the human audible range. But
   // the lower frequencies have a rather faint volume, may not
   // be ideal in flight. Using 3..13 (4096..1170Hz).
   //
   bsteps = center_steps - (pdiff % range_steps); // buzzer steps
   if ( pdiff < 0 ) pdiff *= -1;                  // need positive value now
   nchirps = 1 + (pdiff / range_steps);           // number of chirps.

   //
   // Make sure we make our noises in less than 1s, before the next
   // update comes along.
   //
   if ( nchirps > 50 ) nchirps = 50;   // Wouah, 25m/s - up or down?
   on_time = 500 / nchirps;            // 500ms on time max, same for off time

   start_buzzer_steps( nchirps, 
		       CONV_MS_TO_TICKS( on_time ),
		       CONV_MS_TO_TICKS( on_time ),
		       bsteps );
}

//
// _display_fraction() - display a fraction on the second line, nnnn.nn
// 
// Used to display pressure (in Pascal) as hPa and cm/s as m/s.
//
static void
_display_fraction( s32 value )
{
   u8 *str;
//   u16 m;
   int i;
   int is_neg;

   is_neg = ( value < 0 );
   if ( is_neg )
	value *= -1;

   str = itoa( value, 6, 3 );

   for ( i = 0; (is_neg && (str[i] == ' ')); i++ )
     {
	if (str[i+1] != ' ')
	  str[i] = '-';
     }

   display_chars( LCD_SEG_L2_5_0, str, SEG_ON );

   display_symbol( LCD_SEG_L2_DP, SEG_ON );

}

//
// _display_signed() - display a signed int on the second line, nnnnnn
// 
// Used to display pressure (in Pascal) as hPa and cm/s as m/s.
//
static void
_display_signed( int value )
{
   u8 *str;
   int i;
   int is_neg;
   
   is_neg = ( value < 0 );
   if ( is_neg )
     {
	is_neg = 1;
	value *= -1;
     }

   str = itoa( value, 6, 5 );

   for ( i = 0; (is_neg && (str[i] == ' ')); i++ )
     {
	if (str[i+1] != ' ')
	  str[i] = '-';
     }

   display_chars( LCD_SEG_L2_5_0, str, SEG_ON );

}

//
// Convert barometric value to vz.
// This really depends on altitude and temp, also humidity, but for
// a rough estimation we can take 1Pa = 10cm (0.1m)
//
// TBS -- allow non-metric display...
//
static inline s32
_pascal_to_vz( s32 pa )
{
   return pa * 10;
}

//
// Vario display update function. Theorethically called once per second.
// In practice, this also gets called on button presses, so careful if
// you rely on it for a 1Hz frequency.
//

extern void
display_vario( u8 line, u8 update )
{
   static u8 _idone; // initialisation helper
   static u8 _vbeat; // heartbeat

   u32 pressure;

   switch( update )
     {
      case DISPLAY_LINE_CLEAR:

	stop_buzzer();
	display_symbol( LCD_ICON_BEEPER1, SEG_OFF );
	display_symbol( LCD_ICON_BEEPER2, SEG_OFF );
	display_symbol( LCD_ICON_RECORD,  SEG_OFF );
	display_symbol( LCD_SYMB_MAX,     SEG_OFF );
	return;

      case DISPLAY_LINE_UPDATE_FULL:

	display_symbol( LCD_ICON_BEEPER1,
			( G_vario.beep_mode ) ? SEG_ON : SEG_OFF );

	display_symbol( LCD_ICON_BEEPER2,
			(  ( G_vario.beep_mode == VARIO_BEEPMODE_ASCENT_0 )
			|| ( G_vario.beep_mode == VARIO_BEEPMODE_BOTH ))
			  ? SEG_ON : SEG_OFF );
	//
	// fall through to partial update
	//
      case DISPLAY_LINE_UPDATE_PARTIAL:
	break;
     }
   
   //
   // Partial or full update. Make sure pressure sensor is being sampled, ie,
   // that line 1 is in altimeter mode.
   //

   if ( is_altitude_measurement() )
     {
	u8 *str;
	s16 diff;

	if ( vario_p_read( &pressure ) )
	  {
	     // Happens during key presses, never mind.
	     return; // no data, wait for update
	  }

	//
	// If this is the very first time we are here, we have no previous
	// pressure, handle that situation.
	//
	if ( !_idone  )
	  {
	     diff = 0;
	     ++_idone;
	  }
	else
	  {
	     //
	     // Calculate difference in Pascal - we will need it anyway for the
	     // buzzer. Pressure decreases with altitude, ensure going lower is
	     // negative.
	     // 
	     diff = G_vario.prev_pa - pressure;

	     // update stats as we may want to see these after the flight.

	     if ( diff > G_vario.stats.vzmax ) G_vario.stats.vzmax = diff;
	     if ( diff < G_vario.stats.vzmin ) G_vario.stats.vzmin = diff;
	  }


	// Pulse the vario heartbeat indicator.
	++_vbeat;
	display_symbol( LCD_ICON_RECORD, ( _vbeat & 1 ) ? SEG_ON : SEG_OFF );

	display_symbol( LCD_SYMB_MAX, SEG_OFF);

	// Now see what value to display.

	switch( G_vario.view_mode )
	  {
	   case VARIO_VIEWMODE_ALT_M:
	     //
	     // convert the difference in Pa to a vertical velocity.
	     //
	     _display_fraction( _pascal_to_vz( diff ) );
	     break;

	   case VARIO_VIEWMODE_ALT_PA:
	     //
	     // display raw difference in Pascal.
	     //
	     _display_signed( diff );
	     break;

	   case VARIO_VIEWMODE_PA:
	     //
	     // display pressure as hhhh.pp (hPa and Pa)
	     //
	     _display_fraction( pressure );
	     break;

	   case VARIO_VIEWMODE_VZMAX:
	     display_symbol( LCD_SYMB_MAX, SEG_ON);
	     _display_fraction( _pascal_to_vz( G_vario.stats.vzmax )  );
	     break;

	   case VARIO_VIEWMODE_VZMIN:
	     display_symbol( LCD_SYMB_MAX, SEG_ON);
	     _display_fraction( _pascal_to_vz( G_vario.stats.vzmin ) );
	     break;

	   case VARIO_VIEWMODE_MAX:
	     break;

	  } // switch view mode

	// If beeper is enabled, beep.
	switch ( G_vario.beep_mode )
	  {
	   case VARIO_BEEPMODE_ASCENT_0:
	     if ( diff >= 0 ) chirp( diff );
	     break;
	   case VARIO_BEEPMODE_ASCENT_1:
	     if ( diff > 0 ) chirp( diff );
	     break;
	   case VARIO_BEEPMODE_BOTH:
	     if ( diff ) chirp( diff );
	     break;
	   case VARIO_BEEPMODE_OFF:
	   case VARIO_BEEPMODE_MAX:
	     break;
	  }
	
	// update previous pressure measurement.

	G_vario.prev_pa = pressure;

     } // L1 is in altimeter mode
   else
     {
	display_chars(LCD_SEG_L2_5_0, (u8*) " NOALT", SEG_ON);
     }
}

#endif /* CONFIG_VARIO */
