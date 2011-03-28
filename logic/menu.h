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

#ifndef MENU_H_
#define MENU_H_

// *************************************************************************************************
// Include section


// *************************************************************************************************
// Prototypes section


// *************************************************************************************************
// Defines section

struct menu
{
	// Pointer to direct function (start, stop etc)
	void (*sx_function)(u8 line);		 
	// Pointer to sub menu function (change settings, reset counter etc)
	void (*mx_function)(u8 line);		 
	// Pointer to next item function (normally change to next item)
	void (*nx_function)(u8 line);	
	// Pointer to display function
	void (*display_function)(u8 line, u8 mode);		 
	// Display update trigger 
	u8 (*display_update)(void); 	 
};


// *************************************************************************************************
// Global Variable section


// *************************************************************************************************
// Extern section


extern const struct menu *menu_L1[];
extern const int menu_L1_size;
extern int menu_L1_position;

extern const struct menu *menu_L2[];
extern const int menu_L2_size;
extern int menu_L2_position;


// Line1 navigation
extern const struct menu menu_L1_Time;
#ifdef CONFIG_SIDEREAL
extern const struct menu menu_L1_Sidereal;
#endif
extern const struct menu menu_L1_Alarm;
#ifdef CONFIG_ALTITUDE
extern const struct menu menu_L1_Altitude;
#endif
extern const struct menu menu_L1_Temperature;
extern const struct menu menu_L1_Altitude;
extern const struct menu menu_L1_Heartrate;
extern const struct menu menu_L1_Speed;
extern const struct menu menu_L1_Acceleration;

// Line2 navigation
extern const struct menu menu_L2_Date;
extern const struct menu menu_L2_Stopwatch;
#ifdef CONFIG_EGGTIMER
extern const struct menu menu_L2_Eggtimer;
#endif
extern const struct menu menu_L2_Battery;
extern const struct menu menu_L2_Rf;
#ifdef CONFIG_PHASE_CLOCK
extern const struct menu menu_L2_Phase;
#endif

#ifdef CONFIG_USEPPT
extern const struct menu menu_L2_Ppt;
#endif

#ifndef CONFIG_USE_SYNC_TOSET_TIME
extern const struct menu menu_L2_Sync;
#endif

extern const struct menu menu_L2_CalDist;

#if !defined(CONFIG_BATTERY) || !defined(CONFIG_USE_DISCRET_RFBSL)
extern const struct menu menu_L2_RFBSL;
#endif

#ifdef CONFIG_PROUT
extern const struct menu menu_L2_Prout;
#endif
#ifdef CONFIG_VARIO
extern const struct menu menu_L2_Vario;
#endif

#ifdef CONFIG_USE_GPS
extern const struct menu menu_L2_Gps;
#endif

// Pointers to current menu item
extern const struct menu * ptrMenu_L1;
extern const struct menu * ptrMenu_L2;
#endif /*MENU_H_*/
