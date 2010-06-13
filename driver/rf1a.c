// *************************************************************************************************
//
// Actual revision: $Revision: $
// Revision label:  $Name: $
// Revision state:  $State: $
//
// *************************************************************************************************
// Radio core access functions. Taken from TI reference code for CC430.
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include <project.h>

// driver
#include "rf1a.h"


// *************************************************************************************************
// Global section


// *************************************************************************************************
// Define section
#define st(x)      					do { x } while (__LINE__ == -1)
#define ENTER_CRITICAL_SECTION(x)  	st( x = __get_interrupt_state(); __disable_interrupt(); )
#define EXIT_CRITICAL_SECTION(x)    __set_interrupt_state(x)


// *************************************************************************************************
// @fn          Strobe
// @brief       Send command to radio.
// @param       none
// @return      none
// *************************************************************************************************
unsigned char Strobe(unsigned char strobe)
{
  u8 statusByte = 0;
  u16 int_state, gdo_state;
  
  // Check for valid strobe command 
  if((strobe == 0xBD) || ((strobe > RF_SRES) && (strobe < RF_SNOP)))
  {	     
	  ENTER_CRITICAL_SECTION(int_state);
	  
	  // Clear the Status read flag 
	  RF1AIFCTL1 &= ~(RFSTATIFG);
	  
	  // Wait for radio to be ready for next instruction
	  while( !(RF1AIFCTL1 & RFINSTRIFG));

      // Write the strobe instruction
	  if ((strobe > RF_SRES) && (strobe < RF_SNOP))
	  {
	  	
	  	gdo_state = ReadSingleReg(IOCFG2); // buffer IOCFG2 state
	  	WriteSingleReg(IOCFG2, 0x29); // c-ready to GDO2
	  	
	  	RF1AINSTRB = strobe; 
	  	if ( (RF1AIN&0x04)== 0x04 ) 			// chip at sleep mode
	  	{
 		  	if ( (strobe == RF_SXOFF) || (strobe == RF_SPWD) || (strobe == RF_SWOR) ) { }
 		  	else  	
	  		{
				while ((RF1AIN&0x04)== 0x04);       			// c-ready ?
		 		__delay_cycles(9800);	// Delay for ~810usec at 12MHz CPU clock
	  		}
	  	}
		WriteSingleReg(IOCFG2, gdo_state); // restore IOCFG2 setting
	  }
	  else		// chip active mode
	  {	
		  RF1AINSTRB = strobe; 	   
	  }
	  statusByte = RF1ASTATB;
	  while( !(RF1AIFCTL1 & RFSTATIFG) );
	  EXIT_CRITICAL_SECTION(int_state);
  }
  return statusByte;
}


// *************************************************************************************************
// @fn          ResetRadioCore
// @brief       Software reset radio core.
// @param       none
// @return      none
// *************************************************************************************************
void ResetRadioCore(void)
{
  Strobe(RF_SRES);                             // Reset the Radio Core
  Strobe(RF_SNOP);                             // Reset Radio Pointer
}


// *************************************************************************************************
// @fn          ReadSingleReg
// @brief       Read byte from register.
// @param       none
// @return      none
// *************************************************************************************************
unsigned char ReadSingleReg(unsigned char addr)
{
  unsigned char x;
  u16 int_state;

  ENTER_CRITICAL_SECTION(int_state);

  RF1AINSTR1B = (addr | RF_REGRD); 
  x = RF1ADOUT1B;

  EXIT_CRITICAL_SECTION(int_state);  

  return x;
}


// *************************************************************************************************
// @fn          WriteSingleReg
// @brief       Write byte to register.
// @param       none
// @return      none
// *************************************************************************************************
void WriteSingleReg(unsigned char addr, unsigned char value)
{ 
	volatile unsigned int i;
	u16 int_state;

	ENTER_CRITICAL_SECTION(int_state);
	
    while (!(RF1AIFCTL1 & RFINSTRIFG));     // Wait for the Radio to be ready for the next instruction    
    
    RF1AINSTRW = ((addr | RF_REGWR)<<8 ) + value; // Send address + Instruction
	while (!(RFDINIFG & RF1AIFCTL1));

	i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte

	EXIT_CRITICAL_SECTION(int_state);
}


// *************************************************************************************************
// @fn          ReadBurstReg
// @brief       Read sequence of bytes from register.
// @param       none
// @return      none
// *************************************************************************************************
void ReadBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{
  unsigned int i;
  u16 int_state;

  ENTER_CRITICAL_SECTION(int_state);
  
  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTR1B = (addr | RF_REGRD);          // Send address + Instruction

  for (i = 0; i < (count-1); i++)
  {
    while (!(RFDOUTIFG&RF1AIFCTL1));        // Wait for the Radio Core to update the RF1ADOUTB reg
    buffer[i] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
                                            // Also initiates auo-read for next DOUT byte
  }
  buffer[count-1] = RF1ADOUT0B;             // Store the last DOUT from Radio Core

  EXIT_CRITICAL_SECTION(int_state);
}  
    

// *************************************************************************************************
// @fn          WriteBurstReg
// @brief       Write sequence of bytes to register.
// @param       none
// @return      none
// *************************************************************************************************
void WriteBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{  
  // Write Burst works wordwise not bytewise - bug known already
  unsigned char i;                             
  u16 int_state;

  ENTER_CRITICAL_SECTION(int_state);

  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTRW = ((addr | RF_REGWR)<<8 ) + buffer[0]; // Send address + Instruction

  for (i = 1; i < count; i++)
  {
    RF1ADINB = buffer[i];                   // Send data
    while (!(RFDINIFG & RF1AIFCTL1));       // Wait for TX to finish
  } 
  i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte
   
  EXIT_CRITICAL_SECTION(int_state);
}


// *************************************************************************************************
// @fn          WritePATable
// @brief       Write data to power table
// @param       unsigned char value		Value to write
// @return      none
// *************************************************************************************************
void WritePATable(unsigned char value)
{
  unsigned char readbackPATableValue = 0;
  u16 int_state;

  ENTER_CRITICAL_SECTION(int_state);

  while(readbackPATableValue != value)
  {
    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRW = 0x7E00 + value;               // PA Table write (burst)
  
    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRB = RF_SNOP;                      // reset pointer
  

    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRB = 0xFE;                      // PA Table read (burst)

    while( !(RF1AIFCTL1 & RFDINIFG));
    RF1ADINB    = 0x00;                     //dummy write

    while( !(RF1AIFCTL1 & RFDOUTIFG));
    readbackPATableValue = RF1ADOUT0B;

    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRB = RF_SNOP;
  }

  EXIT_CRITICAL_SECTION(int_state); 
}


