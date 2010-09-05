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

#ifndef SIDEREALTIME_H_
#define SIDEREALTIME_H_

// *************************************************************************************************
// Defines section

// *************************************************************************************************
// Prototypes section
extern void sync_sidereal(void);
extern void reset_sidereal_clock(void);
extern void sx_sidereal(u8 line);
extern void mx_sidereal(u8 line);
extern void sidereal_clock_tick(void);
extern void display_sidereal(u8 line, u8 update);

// *************************************************************************************************
// Global Variable section

//count of different longitudes that can be stored and selected from
#define SIDEREAL_NUM_LON 3

//longitude >0:east, <0:west (the parts are not allowed to differ in sign)
struct longitude
{
	s16		deg;
	s8		min;
	s8		sec;
};


struct sidereal_time
{
	// Flag to minimize display updates
	u8 		drawFlag;

	// Viewing style
	u8		line1ViewStyle;
	u8		line2ViewStyle;
	
	// Time data
	u8		hour;
	u8		minute;
	u8 		second;
	
	//SIDEREAL_NUM_LON different longitudes
	struct longitude lon[SIDEREAL_NUM_LON];
	//selected longitude to use for time calculation
	u8		lon_selection;
	
	//synchronize to normal time automatically
	u8		sync;
};
extern struct sidereal_time sSidereal_time;

#define SIDEREAL_INFOMEM_ID 0x10

#endif /*SIDEREALTIME_H_*/
