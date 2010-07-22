// *************************************************************************************************
//
//	Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/ 
//	Copyright (C) 2010 Daniel Poelzleithner
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

#ifndef PHASE_CLOCK_H_
#ifdef CONFIG_PHASE_CLOCK
#define PHASE_CLOCK_H_

#include "rfsimpliciti.h"
#include "simpliciti.h"

// *************************************************************************************************
// Include section


// *************************************************************************************************
// Prototypes section
extern void reset_sleep(void);
extern void ping_clock(void);
extern void collcet_data(void);
extern void send_data(void);

extern void display_phase_clock(u8 line, u8 update);

extern void sx_phase(u8 line);
extern void mx_phase(u8 line);

extern void phase_clock_calcpoint();


// *************************************************************************************************
// Defines section

// SimpliciTI connection states
typedef enum
{
  SLEEP_OFF = 0,       // Not connected
  SLEEP_CONNECTING,	   // connection in progress
  SLEEP_CONNECTED,	   // connected to clock
} sleep_mode_t;

// collect n samples of data before tranmitting (saves energy)

// n bits in the couter byte which mark the session
#define SLEEP_RF_ID_BIT_LENGHT 3
// 5 bits + 1 so counter%SLEEP_MAX_PACKET_COUNTER does not overflow
#define SLEEP_MAX_PACKET_COUNTER 32 


#define SLEEP_DATA_BUFFER              30
#define SLEEP_OUT_BUFFER               10

// how often should a the clock be searched again
#define SEARCH_CLOCK                         (60*10)
// protocol is PREFIX + ID + PAYLOAD + CHECKSUM

#define SLEEP_ACC_PREFIX                     0x82cf // prefix when transmitting acceleration data
#define SLEEP_TIME_PREFIX                    0x82aa // prefix when setting the clock

//#define PHASE_CLOCK_SEND_LENGTH (SIMPLICITI_MAX_PAYLOAD_LENGTH-2)
#define PHASE_CLOCK_SEND_LENGTH 2
#define PHASE_CLOCK_BUFFER 2
//#define PHASE_CLOCK_BUFFER ((SIMPLICITI_MAX_PAYLOAD_LENGTH-2)/sizeof(u16))

// Button flags for SimpliciTI data
//#define SIMPLICITI_BUTTON_STAR			(0x10)
//#define SIMPLICITI_BUTTON_NUM			(0x20)
//#define SIMPLICITI_BUTTON_UP			(0x30)

// SimpliciTI mode flag
//#define SIMPLICITI_MOUSE_EVENTS			(0x01)
//#define SIMPLICITI_KEY_EVENTS			(0x02)


// *************************************************************************************************
// Global Variable section
struct SPhase
{
    u8                  bug;
	// current session id
    u8                  session;
	// sleep program to start
	u8					program;
    // collected data
    // 
	u8                  data[SLEEP_DATA_BUFFER][3];
    u8                  data_nr;
    //u8                  out[PHASE_CLOCK_BUFFER];
    u16                 out[SLEEP_OUT_BUFFER];
    u8                  out_nr;
};
extern struct SPhase sPhase;

//extern unsigned char simpliciti_flag;

// *************************************************************************************************
// Extern section

#endif /*CONFIG_PHASE_CLOCK*/
#endif /*RFSIMPLICITI_H_*/
