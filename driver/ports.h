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

#ifndef BUTTONS_H_
#define BUTTONS_H_

// *************************************************************************************************
// Defines section

// Port, pins and interrupt resources for buttons
#define BUTTONS_IN              (P2IN)
#define BUTTONS_OUT				(P2OUT)
#define BUTTONS_DIR             (P2DIR)
#define BUTTONS_REN				(P2REN)
#define BUTTONS_IE              (P2IE)
#define BUTTONS_IES             (P2IES)
#define BUTTONS_IFG             (P2IFG)
#define BUTTONS_IRQ_VECT2       (PORT2_VECTOR)

// Button ports
#define BUTTON_STAR_PIN        	(BIT2)
#define BUTTON_NUM_PIN         	(BIT1)
#define BUTTON_UP_PIN          	(BIT4)
#define BUTTON_DOWN_PIN        	(BIT0)
#define BUTTON_BACKLIGHT_PIN   	(BIT3)
#define ALL_BUTTONS			 	(BUTTON_STAR_PIN + BUTTON_NUM_PIN + BUTTON_UP_PIN + BUTTON_DOWN_PIN + BUTTON_BACKLIGHT_PIN)

// Macros for button press detection
#define BUTTON_STAR_IS_PRESSED		((BUTTONS_IN & BUTTON_STAR_PIN) == BUTTON_STAR_PIN)
#define BUTTON_NUM_IS_PRESSED		((BUTTONS_IN & BUTTON_NUM_PIN) == BUTTON_NUM_PIN)
#define BUTTON_UP_IS_PRESSED		((BUTTONS_IN & BUTTON_UP_PIN) == BUTTON_UP_PIN)
#define BUTTON_DOWN_IS_PRESSED		((BUTTONS_IN & BUTTON_DOWN_PIN) == BUTTON_DOWN_PIN)
#define BUTTON_BACKLIGHT_IS_PRESSED	((BUTTONS_IN & BUTTON_BACKLIGHT_PIN) == BUTTON_BACKLIGHT_PIN)
#define NO_BUTTON_IS_PRESSED		((BUTTONS_IN & ALL_BUTTONS) == 0)

// Macros for button release detection
#define BUTTON_STAR_IS_RELEASED			((BUTTONS_IN & BUTTON_STAR_PIN) == 0)
#define BUTTON_NUM_IS_RELEASED			((BUTTONS_IN & BUTTON_NUM_PIN) == 0)
#define BUTTON_UP_IS_RELEASED			(BUTTONS_IN & BUTTON_UP_PIN) == 0)
#define BUTTON_DOWN_IS_RELEASED			((BUTTONS_IN & BUTTON_DOWN_PIN) == 0)
#define BUTTON_BACKLIGHT_IS_RELEASED	((BUTTONS_IN & BUTTON_BACKLIGHT_PIN) == 0)

// Button debounce time (msec)
#define BUTTONS_DEBOUNCE_TIME_IN	(5u)
#define BUTTONS_DEBOUNCE_TIME_OUT	(250u)
#define BUTTONS_DEBOUNCE_TIME_LEFT	(50u)

// Detect if STAR / NUM button is held low continuously  
#define LEFT_BUTTON_LONG_TIME		(2u)

// Leave set_value() function after some seconds of user inactivity
#define INACTIVITY_TIME			(30u)


// Set of button flags
typedef union
{
  struct
  {
  	// Manual button events
    u16 star    	: 1;    // Short STAR button press
    u16 num     	: 1;    // Short NUM button press
    u16 up      	: 1;    // Short UP button press
    u16 down      	: 1;    // Short DOWN button press
    u16 backlight  	: 1;    // Short BACKLIGHT button press
    u16 star_long   : 1;    // Long STAR button press
    u16 num_long   	: 1;    // Long NUM button press
  } flag;
  u16 all_flags;            // Shortcut to all display flags (for reset)
} s_button_flags;
extern volatile s_button_flags button;

struct struct_button
{
	u8  star_timeout;		 
	u8  num_timeout;		 
	s16 repeats;			
};
extern volatile struct struct_button sButton;

// *************************************************************************************************
// Extern section
extern void button_repeat_on(u16 msec);
extern void button_repeat_off(void);
extern void button_repeat_function(void);
extern void init_buttons(void);


#endif /*BUTTONS_H_*/
