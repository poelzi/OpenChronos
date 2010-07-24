/*
    Vario function for ez430 chronos watch.
    Copyright (C) 2010 Marc Poulhiès <dkm@kataplop.net>

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

#ifdef CONFIG_PROUT

// driver
#include "altitude.h"
#include "display.h"
#include "vti_ps.h"
#include "ports.h"
#include "timer.h"

#include "stopwatch.h"

// logic
#include "user.h"
#include "prout.h"

#include "menu.h"

struct prouttimer sprouttimer;

#define POUET_STR " POUET PROUT KATAPLOP  POUET"

static u8 *str = POUET_STR;

void prout_tick()
{
  sprouttimer.pos = (sprouttimer.pos+1) % (sizeof(POUET_STR)-7);
  display_prout(0, 0);
}

u8 is_prout(void)
{
  return (sprouttimer.state == PROUT_RUN &&  (ptrMenu_L2 == &menu_L2_Prout));
}

void update_prout_timer()
{
  /* TA0CCR2 = TA0CCR2 + STOPWATCH_1HZ_TICK; */
}

void start_prout()
{

  sprouttimer.state = PROUT_RUN;
  /* // Init CCR register with current time */
  /* TA0CCR2 = TA0R; */
		
  /* // Load CCR register with next capture time */
  /* update_prout_timer(); */

  /* // Reset IRQ flag     */
  /* TA0CCTL2 &= ~CCIFG;  */
	          
  /* // Enable timer interrupt     */
  /* TA0CCTL2 |= CCIE;  */
	
  display_symbol(LCD_ICON_RECORD, SEG_ON);
}

void stop_prout()
{
  /* // Clear timer interrupt enable    */
  /* TA0CCTL2 &= ~CCIE;  */

  sprouttimer.state = PROUT_STOP;
	
  display_symbol(LCD_ICON_RECORD, SEG_OFF);

  // Call draw routine immediately
  display_prout(LINE2, DISPLAY_LINE_UPDATE_FULL);
}

void sx_prout(u8 line)
{
  if (button.flag.down)
    {
      if (sprouttimer.state == PROUT_STOP){
        start_prout();
      } else {
        stop_prout();
      }
    }
}

void mx_prout(u8 line)
{
}

void display_prout(u8 line, u8 update)
{
  u8 cur[7];
  memcpy(cur, str + sprouttimer.pos, 6);
  cur[6] = 0;
  
  display_chars(LCD_SEG_L2_5_0, cur, SEG_ON);
}

void reset_prout(void)
{
  sprouttimer.pos = 0;
  sprouttimer.state = PROUT_STOP;
}

#endif /* CONFIG_PROUT */
