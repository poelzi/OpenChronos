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
// VTI CMA3000-D0x acceleration sensor driver functions
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// logic
#include "simpliciti.h"

// driver
#include "vti_as.h"
#include "timer.h"
#include "display.h"


// *************************************************************************************************
// Prototypes section
void as_start(void);
void as_stop(void);
u8 as_read_register(u8 bAddress);
u8 as_write_register(u8 bAddress, u8 bData);


// *************************************************************************************************
// Defines section

// =================================================================================================
// CMA3000-D0x acceleration sensor configuration
// =================================================================================================
// DCO frequency division factor determining speed of the acceleration sensor SPI interface
// Speed in Hz = 12MHz / AS_BR_DIVIDER (max. 500kHz)
#define AS_BR_DIVIDER        (30u)

// Acceleration measurement range in g
// Valid ranges are: 2 and 8
#define AS_RANGE             (2u)

// Sample rate for acceleration values in Hz
// Valid sample rates for 2g range are:     100, 400
// Valid sample rates for 8g range are: 40, 100, 400
#define AS_SAMPLE_RATE       (100u)


// *************************************************************************************************
// Global Variable section

// Global flag for proper acceleration sensor operation
u8 as_ok;


// *************************************************************************************************
// Extern section



// *************************************************************************************************
// @fn          as_init
// @brief       Setup acceleration sensor connection, do not power up yet
// @param       none
// @return      none
// *************************************************************************************************
void as_init(void)
{
#ifdef AS_DISCONNECT	
	// Deactivate connection to acceleration sensor
	AS_PWR_OUT &= ~AS_PWR_PIN;            	// Power off
	AS_INT_OUT &= ~AS_INT_PIN;            	// Pin to low to avoid floating pins
	AS_SPI_OUT &= ~(AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN); // Pin to low to avoid floating pins
	AS_CSN_OUT &= ~AS_CSN_PIN; 				// Pin to low to avoid floating pins
	AS_INT_DIR |= AS_INT_PIN;            	// Pin to output to avoid floating pins
	AS_SPI_DIR |= AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN;    // Pin to output to avoid floating pins
	AS_CSN_DIR |= AS_CSN_PIN;				// Pin to output to avoid floating pins
	AS_PWR_DIR |= AS_PWR_PIN;            	// Power pin to output direction
#else
	AS_INT_DIR &= ~AS_INT_PIN;            	// Input
	AS_SPI_DIR &= ~AS_SDI_PIN; 				// Input
	AS_SPI_DIR |= AS_SDO_PIN + AS_SCK_PIN;  // Output
	AS_SPI_SEL |= AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN; // Port pins to SDO, SDI and SCK function
	AS_CSN_OUT |= AS_CSN_PIN; 				// CSN=1
	AS_CSN_DIR |= AS_CSN_PIN;				// 
	AS_PWR_OUT |= AS_PWR_PIN;            	// VDD=1
	AS_PWR_DIR |= AS_PWR_PIN;            	// 
#endif

	// Reset global sensor flag
	as_ok = 1;
}


// *************************************************************************************************
// @fn          as_start
// @brief       Power-up and initialize acceleration sensor
// @param       none
// @return      none
// *************************************************************************************************
void as_start(void)
{
	volatile u16 Counter_u16;
	u8 bConfig;//, bStatus;
	
	// Initialize SPI interface to acceleration sensor
	AS_SPI_CTL0 |= UCSYNC | UCMST | UCMSB // SPI master, 8 data bits,  MSB first,
	               | UCCKPH;              //  clock idle low, data output on falling edge
	AS_SPI_CTL1 |= UCSSEL1;               // SMCLK as clock source
	AS_SPI_BR0   = AS_BR_DIVIDER;         // Low byte of division factor for baud rate
	AS_SPI_BR1   = 0x00;                  // High byte of division factor for baud rate
	AS_SPI_CTL1 &= ~UCSWRST;              // Start SPI hardware
  
	// Initialize interrupt pin for data read out from acceleration sensor
	AS_INT_IES &= ~AS_INT_PIN;            // Interrupt on rising edge

#ifdef AS_DISCONNECT	  
	// Enable interrupt 
	AS_INT_DIR &= ~AS_INT_PIN;            // Switch INT pin to input
	AS_SPI_DIR &= ~AS_SDI_PIN;            // Switch SDI pin to input
	AS_SPI_REN |=  AS_SDI_PIN;            // Pulldown on SDI pin
	AS_SPI_SEL |=  AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN; // Port pins to SDO, SDI and SCK function
	AS_CSN_OUT |=  AS_CSN_PIN;            // Deselect acceleration sensor
	AS_PWR_OUT |=  AS_PWR_PIN;            // Power on active high
#endif

	// Delay of >5ms required between switching on power and configuring sensor
	Timer0_A4_Delay(CONV_MS_TO_TICKS(10));
	
	// Initialize interrupt pin for data read out from acceleration sensor
	AS_INT_IFG &= ~AS_INT_PIN;            // Reset flag
	AS_INT_IE  |=  AS_INT_PIN;            // Enable interrupt
	
	// Configure sensor and start to sample data
#if (AS_RANGE == 2)
  bConfig = 0x80;
  #if (AS_SAMPLE_RATE == 100)
    bConfig |= 0x02;
  #elif (AS_SAMPLE_RATE == 400)
    bConfig |= 0x04;
  #else
    #error "Sample rate not supported"
  #endif
#elif (AS_RANGE == 8)
  bConfig = 0x00;
  #if (AS_SAMPLE_RATE == 40)
    bConfig |= 0x06;
  #elif (AS_SAMPLE_RATE == 100)
    bConfig |= 0x02;
  #elif (AS_SAMPLE_RATE == 400)
    bConfig |= 0x04;
  #else
    #error "Sample rate not supported"
  #endif
#else
  #error "Measurement range not supported"    
#endif  

	// Reset sensor
	as_write_register(0x04, 0x02);   
	as_write_register(0x04, 0x0A);   
	as_write_register(0x04, 0x04);   
	
	// Wait 5 ms before starting sensor output
	Timer0_A4_Delay(CONV_MS_TO_TICKS(5));
	
	// Set 2g measurement range, start to output data with 100Hz rate
	as_write_register(0x02, bConfig);   
}



// *************************************************************************************************
// @fn          as_stop
// @brief       Power down acceleration sensor
// @param       none
// @return      none
// *************************************************************************************************
void as_stop(void)
{
	// Disable interrupt 
	AS_INT_IE  &=  ~AS_INT_PIN;            	// Disable interrupt

#ifdef AS_DISCONNECT
	// Power-down sensor
	AS_PWR_OUT &= ~AS_PWR_PIN;            	// Power off
	AS_INT_OUT &= ~AS_INT_PIN;            	// Pin to low to avoid floating pins
	AS_SPI_OUT &= ~(AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN); // Pins to low to avoid floating pins
	AS_SPI_SEL &= ~(AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN); // Port pins to I/O function
	AS_CSN_OUT &= ~AS_CSN_PIN; 				// Pin to low to avoid floating pins
	AS_INT_DIR |= AS_INT_PIN;            	// Pin to output to avoid floating pins
	AS_SPI_DIR |= AS_SDO_PIN + AS_SDI_PIN + AS_SCK_PIN;    // Pins to output to avoid floating pins
	AS_CSN_DIR |= AS_CSN_PIN;				// Pin to output to avoid floating pins
#else
	// Reset sensor -> sensor to powerdown
	as_write_register(0x04, 0x02);   
	as_write_register(0x04, 0x0A);   
	as_write_register(0x04, 0x04);   
#endif
}


// *************************************************************************************************
// @fn          as_read_register
// @brief       Read a byte from the acceleration sensor
// @param       u8 bAddress		Register address
// @return      u8					Register content
// *************************************************************************************************
u8 as_read_register(u8 bAddress)
{
  u8 bResult;
  u16 timeout;
  
  // Exit function if an error was detected previously
  if (!as_ok) return (0);

  bAddress <<= 2;                     // Address to be shifted left by 2 and RW bit to be reset

  AS_SPI_REN &= ~AS_SDI_PIN;          // Pulldown on SDI pin not required
  AS_CSN_OUT &= ~AS_CSN_PIN;          // Select acceleration sensor

  bResult = AS_RX_BUFFER;             // Read RX buffer just to clear interrupt flag

  AS_TX_BUFFER = bAddress;            // Write address to TX buffer
  
  timeout = SPI_TIMEOUT;
  while (!(AS_IRQ_REG & AS_RX_IFG) && (--timeout>0));  // Wait until new data was written into RX buffer
  if (timeout == 0)
  {
  	as_ok = 0;
  	return (0);
  }
  bResult = AS_RX_BUFFER;             // Read RX buffer just to clear interrupt flag

  AS_TX_BUFFER = 0;                   // Write dummy data to TX buffer
  
  timeout = SPI_TIMEOUT;
  while (!(AS_IRQ_REG & AS_RX_IFG) && (--timeout>0));  // Wait until new data was written into RX buffer
  if (timeout == 0)
  {
  	as_ok = 0;
  	return (0);
  }
  bResult = AS_RX_BUFFER;             // Read RX buffer

  AS_CSN_OUT |=  AS_CSN_PIN;          // Deselect acceleration sensor
  AS_SPI_REN |=  AS_SDI_PIN;          // Pulldown on SDI pin required again

  // Return new data from RX buffer
  return bResult;
}


// *************************************************************************************************
// @fn          as_write_register
// @brief  		Write a byte to the acceleration sensor
// @param       u8 bAddress		Register address
//				u8 bData			Data to write
// @return      u8					
// *************************************************************************************************
u8 as_write_register(u8 bAddress, u8 bData)
{
  u8 bResult;
  u16 timeout;
  
  // Exit function if an error was detected previously
  if (!as_ok) return (0);

  bAddress <<= 2;                     // Address to be shifted left by 1
  bAddress |= BIT1;                   // RW bit to be set
  
  AS_SPI_REN &= ~AS_SDI_PIN;          // Pulldown on SDI pin not required
  AS_CSN_OUT &= ~AS_CSN_PIN;          // Select acceleration sensor

  bResult = AS_RX_BUFFER;             // Read RX buffer just to clear interrupt flag

  AS_TX_BUFFER = bAddress;            // Write address to TX buffer
  
  timeout = SPI_TIMEOUT;
  while (!(AS_IRQ_REG & AS_RX_IFG) && (--timeout>0));  // Wait until new data was written into RX buffer
  if (timeout == 0)
  {
  	as_ok = 0;
  	return (0);
  }
  bResult = AS_RX_BUFFER;             // Read RX buffer just to clear interrupt flag

  AS_TX_BUFFER = bData;               // Write data to TX buffer

  timeout = SPI_TIMEOUT;
  while (!(AS_IRQ_REG & AS_RX_IFG) && (--timeout>0));  // Wait until new data was written into RX buffer
  if (timeout == 0) 
  {
  	as_ok = 0;
  	return (0);
  }
  bResult = AS_RX_BUFFER;             // Read RX buffer

  AS_CSN_OUT |=  AS_CSN_PIN;          // Deselect acceleration sensor
  AS_SPI_REN |=  AS_SDI_PIN;          // Pulldown on SDI pin required again

  return bResult;
}


// *************************************************************************************************
// @fn          as_get_data
// @brief       Service routine to read acceleration values.
// @param       none
// @return      none
// *************************************************************************************************
void as_get_data(u8 * data)
{
	// Exit if sensor is not powered up
	if ((AS_PWR_OUT & AS_PWR_PIN) != AS_PWR_PIN) return;
  
  	// Store X/Y/Z acceleration data in buffer
	*(data+0) = as_read_register(0x06);
	*(data+1) = as_read_register(0x07);
	*(data+2) = as_read_register(0x08);
}


