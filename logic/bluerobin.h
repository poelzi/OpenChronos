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

#ifndef BLUEROBIN_H_
#define BLUEROBIN_H_


// *************************************************************************************************
// Include section
#include <project.h>


// *************************************************************************************************
// Prototypes section
extern void reset_bluerobin(void);
extern void mx_bluerobin(u8 line);
extern void sx_bluerobin(u8 line);
extern void mx_caldist(u8 line);
extern void display_heartrate(u8 line, u8 update);
extern void display_speed(u8 line, u8 update);
extern void sx_caldist(u8 line);
extern void mx_caldist(u8 line);
extern void display_caldist(u8 line, u8 update);
extern u8 is_bluerobin(void);
extern u8 is_bluerobin_searching(void);
extern void get_bluerobin_data(void);
extern void stop_bluerobin(void);


// *************************************************************************************************
// Defines section

// BlueRobin connection states
typedef enum
{
  BLUEROBIN_OFF = 0,        // Not connected
  BLUEROBIN_SEARCHING,      // Searching for transmitter
  BLUEROBIN_CONNECTED,		// Connected
  BLUEROBIN_ERROR			// Error occurred while trying to connect or while connected
} BlueRobin_state_t;

// BlueRobin data update states
typedef enum
{
  BLUEROBIN_NO_UPDATE = 0,   // No new data available
  BLUEROBIN_NEW_DATA       	// New data arrived
} BlueRobin_update_t;

#define USER_SEX_MALE				0
#define USER_SEX_FEMALE				1
#define USER_WEIGHT_MIN_KG			30
#define USER_WEIGHT_MAX_KG			150
#define USER_WEIGHT_MIN_LB			70
#define USER_WEIGHT_MAX_LB			400


// *************************************************************************************************
// Global Variable section
struct br
{
	// BLUEROBIN_OFF, BLUEROBIN_SEARCHING, BLUEROBIN_CONNECTED, BLUEROBIN_ERROR
	BlueRobin_state_t 	state;
	
	// BLUEROBIN_NO_UPDATE, BLUEROBIN_NEW_DATA
	BlueRobin_update_t	update;
	
	// Chest strap ID	
	u32	cs_id;

	// User settings
	u8 		user_sex;
	u16		user_weight;
	
	// Heart rate (1 bpm)
	u8 		heartrate;
	
	// Calories (1 kCal) - calculated from heart rate, user weight and user sex
	u32 	calories;
	
	// Speed (0.1 km/h) - demo version range is 0.0 to 25.5km/h 
	u8 		speed;
	
	// Distance (1 m)
	u32 	distance;
	
	// 0=display calories, 1=display distance
	u8		caldist_view;
};
extern struct br sBlueRobin;

// *************************************************************************************************
// Extern section

#endif /*BLUEROBIN_H_*/
