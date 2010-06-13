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

#ifndef ALTITUDE_H_
#define ALTITUDE_H_


// *************************************************************************************************
// Include section


// *************************************************************************************************
// Prototypes section
extern void reset_altitude_measurement(void);
extern u8 is_altitude_measurement(void);
extern void start_altitude_measurement(void);
extern void stop_altitude_measurement(void);
extern void do_altitude_measurement(u8 filter);

// menu functions
extern void sx_altitude(u8 line);
extern void mx_altitude(u8 line);
extern void display_altitude(u8 line, u8 update);


// *************************************************************************************************
// Defines section

// Stop altitude measurement after 60 minutes to save battery
#define ALTITUDE_MEASUREMENT_TIMEOUT	(60*60u)


// *************************************************************************************************
// Global Variable section
struct alt
{
	// MENU_ITEM_NOT_VISIBLE, MENU_ITEM_VISIBLE   
	menu_t	state; 

	// Pressure (Pa)
	u32		pressure;

	// Temperature (°K)
	u16		temperature;

	// Altitude (m)
	s16		altitude;
	
	// Altitude offset stored during calibration
	s16		altitude_offset;

	// Timeout
	u16		timeout;
};
extern struct alt sAlt;


// *************************************************************************************************
// Extern section


#endif /*ALTITUDE_H_*/
