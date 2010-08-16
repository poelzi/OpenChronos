// *************************************************************************************************
//
// Copyright 2009 BM innovations GmbH (www.bm-innovations.com), all rights reserved.
//
// This trial version of the "BlueRobin(TM) receiver library for the Texas Instruments 
// CC430 SoC" may be used for non-profit non-commercial purposes only. If you want to use 
// BlueRobin(TM) in a commercial project, please contact the copyright holder for a 
// separate license agreement.  
//
// By using this trial version of the "BlueRobin(TM) receiver library for the Texas Instruments 
// CC430 SoC", you implicitly agree that you will not modify, adapt, disassemble, decompile, 
// reverse engineer, translate or otherwise attempt to discover the source code of the 
// "BlueRobin(TM) receiver library for the Texas Instruments CC430 SoC".
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
//
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF 
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// *************************************************************************************************
//
// Public header for eZ430-Chronos specific BlueRobin(TM) receiver library.
//
// The following BlueRobin(TM) profiles are supported by this build
//   - heart rate (HR) transmitter
//
// The following number of channels is supported: 1
//
// *************************************************************************************************
//
// BlueRobin(TM) packet size
// -------------------------
// 
// 		* average packet rate	1 packet/875 msec = ~1.14 packets/second				
//		* payload per packet	5 bytes 
//
// BlueRobin(TM) frequency overview 
// (Please note: Settings apply for the transmitter side, i.e. the USB dongle)
// ----------------------------------------------------------------------
//
// Bluerobin_RX_433MHz.lib (433MHz ISM band)
//
//		* frequency				433.30 MHz - 434.00 MHz
//		* deviation				95 kHz
//		* channels				3
//		* data rate				250 kBaud
//
// Bluerobin_RX_868MHz.lib (868MHz ISM band)
//
//		* frequency				868.25 MHz - 868.95 MHz
//		* deviation				95 kHz
//		* channels				3
//		* data rate				250 kBaud
//
//
// Bluerobin_RX_915MHz.lib (915MHz ISM band)
//
//		* frequency				914.35 MHz - 917.75 MHz
//		* deviation				95 kHz
//		* channels				34
//		* data rate				250 kBaud
//
// *************************************************************************************************

#ifndef BRRX_API_H_
#define BRRX_API_H_

// *************************************************************************************************
// Include section


// *************************************************************************************************
// Defines section

// List of all possible channel states
typedef enum 
{
  TX_OFF = 0,                // Powerdown mode
  TX_ACTIVE,                 // Active mode
  TX_SEARCH                  // Search mode
} brtx_state_t;

// Transmitter to channel assignment
#define HR_CHANNEL           (0)


// *************************************************************************************************
// API section

// ----------------------------------------------------------
// Functions for initializing and controlling the library

// Initialize several global variables.
void  BRRX_Init_v(void);

// Set delay after which a channel will be switched off if no new data can be received.
// Param1: Powerdown delay in packet intervals (875 ms)
void  BRRX_SetPowerdownDelay_v(u8 Delay_u8); 

// Set timeout when searching for a transmitter
// Param1: Search timeout in seconds
void  BRRX_SetSearchTimeout_v(u8 Timeout_u8);

// Set reduction of valid signal level in learn mode.
// Param1: Reduction of signal level
void  BRRX_SetSignalLevelReduction_v(u8 Reduction_u8);

// Set ID for a channel. To search for an unknown transmitter the ID has to be set to 0.
// Can be only executed on channels currently in powerdown mode.
// Param1: Channel index
// Param2: New ID
void  BRRX_SetID_v(u8 Index_u8, u32 ID_u32);

// Get current ID of channel.
// Return: Current ID of channel
// Param1: Channel index
u32 BRRX_GetID_u32(u8 Index_u8);

// Start reception on one or all channels.
// Param1: Channel index (use 0xFF to start all channels)
void BRRX_Start_v(u8 Index_u8);

// Stop reception on one or all channels.
// Param1: Channel index (0xFF for all channels)
void BRRX_Stop_v(u8 Index_u8);

// Get current state of a channel
// Param1: Channel index
brtx_state_t BRRX_GetState_t(u8 Index_u8);


// ----------------------------------------------------------
// eZ430-Chronos specific functions

// Get current heart rate.
// Return: Heart rate in bpm
u8  BRRX_GetHeartRate_u8(void);

// Get current distance.
// Return: Distance in 10m steps.
u16 BRRX_GetDistance_u16(void);

// Get current speed.
// Return: Speed in 0.1km/h steps. Trial version is limited to 25.5km/h.
u8  BRRX_GetSpeed_u8(void);

// ----------------------------------------------------------
// Radio-related functions

// RX packet end service function
// Must be called by CC1101_VECTOR ISR
void BlueRobin_RadioISR_v(void);


#endif /*BRRX_API_H_*/
