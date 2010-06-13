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

#ifndef VTI_AS_H_
#define VTI_AS_H_


// *************************************************************************************************
// Include section


// *************************************************************************************************
// Prototypes section
extern void as_init(void);
extern void as_start(void);
extern void as_stop(void);
extern u8 as_read_register(u8 bAddress);
extern u8 as_write_register(u8 bAddress, u8 bData);
extern void as_get_data(u8 * data);


// *************************************************************************************************
// Defines section

// Disconnect power supply for acceleration sensor when not used
#define AS_DISCONNECT

// Port and pin resource for SPI interface to acceleration sensor
// SDO=MOSI=P1.6, SDI=MISO=P1.5, SCK=P1.7
#define AS_SPI_IN            (P1IN)
#define AS_SPI_OUT           (P1OUT)
#define AS_SPI_DIR           (P1DIR)
#define AS_SPI_SEL           (P1SEL)
#define AS_SPI_REN           (P1REN)
#define AS_SDO_PIN           (BIT6)
#define AS_SDI_PIN           (BIT5)
#define AS_SCK_PIN           (BIT7)

// CSN=PJ.1
#define AS_CSN_OUT			 (PJOUT)
#define AS_CSN_DIR			 (PJDIR)
#define AS_CSN_PIN           (BIT1)

#define AS_TX_BUFFER         (UCA0TXBUF)
#define AS_RX_BUFFER         (UCA0RXBUF)
#define AS_TX_IFG            (UCTXIFG)
#define AS_RX_IFG            (UCRXIFG)
#define AS_IRQ_REG           (UCA0IFG) 
#define AS_SPI_CTL0          (UCA0CTL0)
#define AS_SPI_CTL1          (UCA0CTL1) 
#define AS_SPI_BR0           (UCA0BR0)
#define AS_SPI_BR1           (UCA0BR1)

// Port and pin resource for power-up of acceleration sensor, VDD=PJ.0
#define AS_PWR_OUT           (PJOUT)
#define AS_PWR_DIR           (PJDIR)
#define AS_PWR_PIN           (BIT0)

// Port, pin and interrupt resource for interrupt from acceleration sensor, CMA_INT=P2.5
#define AS_INT_IN            (P2IN)
#define AS_INT_OUT           (P2OUT)
#define AS_INT_DIR           (P2DIR)
#define AS_INT_IE            (P2IE)
#define AS_INT_IES           (P2IES)
#define AS_INT_IFG           (P2IFG)
#define AS_INT_PIN           (BIT5)

// SPI timeout to detect sensor failure
#define SPI_TIMEOUT				(1000u)


// *************************************************************************************************
// Global Variable section


// *************************************************************************************************
// Extern section


#endif /*VTI_AS_H_*/
