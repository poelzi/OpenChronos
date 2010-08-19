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
// SimpliciTI functions.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

#ifdef CONFIG_PHASE_CLOCK

// driver
#include <string.h>
#include "display.h"
#include "vti_as.h"
#include "ports.h"
#include "timer.h"
#include "radio.h"

// logic
#include "acceleration.h"
#include "rfsimpliciti.h"
//#include "bluerobin.h"
#include "simpliciti.h"
#include "phase_clock.h"
#include "date.h"
#include "alarm.h"
#include "temperature.h"
#include "vti_ps.h"
#include "altitude.h"
#include "user.h"


// *************************************************************************************************
// Prototypes section
void simpliciti_get_data_callback(void);
void start_simpliciti_sleep();
void start_simpliciti_sync(void);


// *************************************************************************************************
// Defines section
#define TEST

// Each packet index requires 2 bytes, so we can have 9 packet indizes in 18 bytes usable payload
#define BM_SYNC_BURST_PACKETS_IN_DATA		(9u)


// *************************************************************************************************
// Global Variable section
struct SPhase sPhase;

// flag contains status information, trigger to send data and trigger to exit SimpliciTI
unsigned char phase_clock_flag;

// 4 data bytes to send 
unsigned char phase_clock_data[SIMPLICITI_MAX_PAYLOAD_LENGTH];

// 4 byte device address overrides SimpliciTI end device address set in "smpl_config.dat"
unsigned char phase_clock_ed_address[4];

// 1 = send one or more reply packets, 0 = no need to reply
//unsigned char simpliciti_reply;
unsigned char phase_clock_reply_count;

// 1 = send packets sequentially from burst_start to burst_end, 2 = send packets addressed by their index
//u8 		burst_mode;

// Start and end index of packets to send out
//u16		burst_start, burst_end;

// Array containing requested packets
//u16		burst_packet[BM_SYNC_BURST_PACKETS_IN_DATA];

// Current packet index
//u8		burst_packet_index;


// *************************************************************************************************
// Extern section


// *************************************************************************************************
// @fn          sx_sleep
// @brief       Start Sleep mode. Button DOWN connects/disconnects to access point.
// @param       u8 line		LINE2
// @return      none
// *************************************************************************************************
void sx_phase(u8 line)
{
	// Exit if battery voltage is too low for radio operation
	if (sys.flag.low_battery) return;

    sPhase.session = 0;
    sPhase.out_nr = 0;
    sPhase.data_nr = 0;

	// Exit if BlueRobin stack is active
#ifndef ELIMINATE_BLUEROBIN
	if (is_bluerobin()) return;
#endif
  	// Start SimpliciTI in tx only mode
    if(sPhase.bug)
        start_simpliciti_tx_only(SIMPLICITI_PHASE_CLOCK);
    else
        start_simpliciti_tx_only(SIMPLICITI_PHASE_CLOCK_START);
    //start_simpliciti_tx_only(SIMPLICITI_PHASE_CLOCK);
}

// *************************************************************************************************
// @fn          mx_phase
// @brief       Set program number to use
// @param       u8 line		LINE2
// @return      none
// *************************************************************************************************
void mx_phase(u8 line){
		s32 prog, bug;
        u8 mode = 0;
		prog = (s32)sPhase.program;
        bug = (s32)sPhase.bug;
		// Loop values until all are set or user breaks	set
		while(1) 
		{
			// Idle timeout: exit without saving 
			if (sys.flag.idle_timeout) break;
		
			// M2 (short): save, then exit 
			if (button.flag.num) 
			{
				// Store local variables in global Eggtimer default
				//sAlarm.hour = hours;
				//sAlarm.minute = minutes;
				sPhase.program = (u8)prog;
                sPhase.bug = (u8)bug;
				display.flag.line2_full_update = 1;
				break;
			}
			if (button.flag.star) 
                mode = (mode+1)%2;

            switch (mode) {
                case 0:
                    //set_value(&prog, 2, 0, 0, 99, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
                    display_chars(LCD_SEG_L2_5_0, (u8 *)" PR ", SEG_ON);
                    set_value(&prog, 2, 0, 0, 99, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
                    break;
                case 1:
                    display_chars(LCD_SEG_L2_5_0, (u8 *)" BUG", SEG_ON);
                    set_value(&bug, 2, 0, 0, 1, SETVALUE_ROLLOVER_VALUE + SETVALUE_DISPLAY_VALUE + SETVALUE_NEXT_VALUE, LCD_SEG_L2_1_0, display_value1);
                    break;
            }
		}
	
		// Clear button flag
		button.all_flags = 0;
		display_phase_clock(line, DISPLAY_LINE_UPDATE_FULL);
}



// *************************************************************************************************
// @fn          diff
// @brief       calculates the smallest difference between two numbers
// @param       none
// @return      none
// *************************************************************************************************
static u8 diff(u8 x1, u8 x2) {
    u8 b1 = x1 - x2;
    if(b1 > 127)
        b1 = x2 - x1;
    // high pass filter
    if (b1 < 2)
        return 0;
    return b1;
}

// *************************************************************************************************
// @fn          phase_clock_calcpoint
// @brief       calculate one data point for the out buffer
// @param       none
// @return      none
// *************************************************************************************************
void phase_clock_calcpoint() {
	u16 x,y,z,res;
	x = y = z = res = 0;

	u8 i = 0;
	for(i=1;i<SLEEP_DATA_BUFFER;i++) {
		x += diff(sPhase.data[i-1][0], sPhase.data[i][0]);
		y += diff(sPhase.data[i-1][1], sPhase.data[i][1]);
		z += diff(sPhase.data[i-1][2], sPhase.data[i][2]);
	}
	// can't overflow when SLEEP_BUFFER is not larger then 171
	res = x + y + z;

	// set the result into the out buffer
	sPhase.out[sPhase.out_nr] = res;
	sPhase.out_nr++;

	sPhase.data_nr = 0;

}


// *************************************************************************************************
// @fn          display_phase_clock
// @brief       SimpliciTI display routine. 
// @param       u8 line			LINE2
//				u8 update		DISPLAY_LINE_UPDATE_FULL
// @return      none
// *************************************************************************************************
void display_phase_clock(u8 line, u8 update)
{
	if (update == DISPLAY_LINE_UPDATE_FULL)	
	{
		display_chars(LCD_SEG_L2_5_0, (u8 *)" SLEEP", SEG_ON);
	}
}


#endif /*CONFIG_PHASE_CLOCK*/