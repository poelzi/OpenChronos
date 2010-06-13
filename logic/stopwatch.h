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

#ifndef STOPWATCH_H_
#define STOPWATCH_H_

// *************************************************************************************************
// Include section
#include <project.h>


// *************************************************************************************************
// Prototypes section
extern void start_stopwatch(void);
extern void stop_stopwatch(void);
extern void reset_stopwatch(void);
extern u8 is_stopwatch(void);
extern void stopwatch_tick(void);
extern void update_stopwatch_timer(void);
extern void mx_stopwatch(u8 line);
extern void sx_stopwatch(u8 line);
extern void display_stopwatch(u8 line, u8 update);


// *************************************************************************************************
// Defines section
#define STOPWATCH_1HZ_TICK			(32768/1)
#define STOPWATCH_100HZ_TICK		(32768/100)
#define STOPWATCH_STOP				(0u)
#define STOPWATCH_RUN				(1u)
#define STOPWATCH_HIDE				(2u)


// *************************************************************************************************
// Global Variable section
struct stopwatch
{
	u8 		state;
	u8		drawFlag;
	u8		swtIs1Hz;
	u8		swtIs10Hz;
	
	//	time[0] 	hour H
	//	time[1] 	hour L
	//	time[2] 	minute H
	//	time[3] 	minute L
	//	time[4] 	second H
	//	time[5] 	second L
	//	time[6] 	1/10 sec 
	//	time[7] 	1/100 sec
	u8		time[8];
	
	// Display style
	u8 	viewStyle;
};
extern struct stopwatch sStopwatch;


// *************************************************************************************************
// Extern section


#endif /*STOPWATCH_H_*/
