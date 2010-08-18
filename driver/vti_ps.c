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
// VTI SCP1000-D0x pressure sensor driver functions
// *************************************************************************************************


// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "vti_ps.h"
#include "timer.h"
#ifdef FIXEDPOINT
#include "dsp.h"
#endif


// *************************************************************************************************
// Prototypes section
u16 ps_read_register(u8 address, u8 mode);
u8 ps_write_register(u8 address, u8 data);
u8 ps_twi_read(u8 ack);
void twi_delay(void);


// *************************************************************************************************
// Defines section


// *************************************************************************************************
// Global Variable section

#ifndef FIXEDPOINT
// VTI pressure (hPa) to altitude (m) conversion tables
const s16 h0[17] = { -153, 0, 111, 540, 989, 1457, 1949, 2466, 3012, 3591, 4206, 4865, 5574, 6344, 7185, 8117, 9164 };
const u16 p0[17] = { 1031, 1013, 1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300 };
float p[17];
#else
// Storage for pressure to altitude conversions
static s16 pLast; // Last measured pressure in 4Pa units
static s16 pRef; // Reference pressure at sea level in 4Pa units
static s16 hLast; // Last altitude estimate in normalized units b/H0/2^15
#endif


// Global flag for proper pressure sensor operation
u8 ps_ok;


// *************************************************************************************************
// Extern section



// *************************************************************************************************
// @fn          ps_init
// @brief       Init pressure sensor I/O
// @param       none
// @return      none
// *************************************************************************************************
void ps_init(void)
{
	volatile u8 success, status, eeprom, timeout;
	
	PS_INT_DIR &= ~PS_INT_PIN;            	// DRDY is input
	PS_INT_IES &= ~PS_INT_PIN;				// Interrupt on DRDY rising edge
	PS_TWI_OUT |= PS_SCL_PIN + PS_SDA_PIN; 	// SCL and SDA are outputs by default
	PS_TWI_DIR |= PS_SCL_PIN + PS_SDA_PIN; 	// SCL and SDA are outputs by default
	
	// Reset global ps_ok flag
	ps_ok = 0;

	// 100msec delay to allow VDD stabilisation
	Timer0_A4_Delay(CONV_MS_TO_TICKS(100));

	// Reset pressure sensor -> powerdown sensor
	success = ps_write_register(0x06, 0x01);   

	// 100msec delay 
	Timer0_A4_Delay(CONV_MS_TO_TICKS(100));

	// Check if STATUS register BIT0 is cleared
	status = ps_read_register(0x07, PS_TWI_8BIT_ACCESS);
	if (((status & BIT0) == 0) && (status != 0)) 
	{
		// Check EEPROM checksum in DATARD8 register 
		eeprom = ps_read_register(0x7F, PS_TWI_8BIT_ACCESS);
		if (eeprom == 0x01) ps_ok = 1;
		else 				ps_ok = 0;
	}
}


// *************************************************************************************************
// @fn          ps_start
// @brief       Init pressure sensor registers and start sampling
// @param       none
// @return      u8		1=Sensor started, 0=Sensor did not start
// *************************************************************************************************
void ps_start(void)
{
	// Start sampling data in ultra low power mode 
	ps_write_register(0x03, 0x0B);  
}



// *************************************************************************************************
// @fn          ps_stop
// @brief       Power down pressure sensor
// @param       none
// @return      none
// *************************************************************************************************
void ps_stop(void)
{
	// Put sensor to standby
	ps_write_register(0x03, 0x00);   
}



// *************************************************************************************************
// @fn          ps_twi_sda
// @brief       Control SDA line
// @param       u8 condition		PS_TWI_SEND_START, PS_TWI_SEND_RESTART, PS_TWI_SEND_STOP
//										PS_TWI_CHECK_ACK
// @return      u8					1=ACK, 0=NACK
// *************************************************************************************************
u8 ps_twi_sda(u8 condition)
{
	u8 sda = 0;
	
	if (condition == PS_TWI_SEND_START)
	{
		PS_TWI_SDA_OUT;			// SDA is output
		PS_TWI_SCL_HI;
		twi_delay();
		PS_TWI_SDA_LO;
		twi_delay();
		PS_TWI_SCL_LO;			// SCL 1-0 transition while SDA=0
		twi_delay();
	}
	else if (condition == PS_TWI_SEND_RESTART)
	{
		PS_TWI_SDA_OUT;			// SDA is output
		PS_TWI_SCL_LO;
		PS_TWI_SDA_HI;
		twi_delay();
		PS_TWI_SCL_HI;
		twi_delay();
		PS_TWI_SDA_LO;
		twi_delay();
		PS_TWI_SCL_LO;
		twi_delay();
	}
	else if (condition == PS_TWI_SEND_STOP)
	{
		PS_TWI_SDA_OUT;			// SDA is output
		PS_TWI_SDA_LO;
		twi_delay();
		PS_TWI_SCL_LO;		
		twi_delay();
		PS_TWI_SCL_HI;			
		twi_delay();
		PS_TWI_SDA_HI;			// SDA 0-1 transition while SCL=1
		twi_delay();
	}
	else if (condition == PS_TWI_CHECK_ACK)
	{
		PS_TWI_SDA_IN;			// SDA is input
		PS_TWI_SCL_LO;		
		twi_delay();
		PS_TWI_SCL_HI;			
		twi_delay();
		sda = PS_TWI_IN & PS_SDA_PIN;
		PS_TWI_SCL_LO;			
	}
	
	// Return value will only be evaluated when checking device ACK
	return (sda == 0);
}



// *************************************************************************************************
// @fn          twi_delay
// @brief       Delay between TWI signal edges. 
// @param       none
// @return      none
// *************************************************************************************************
void twi_delay(void)
{
	asm("	nop");
}


// *************************************************************************************************
// @fn          ps_twi_write
// @brief       Clock out bits through SDA.
// @param       u8 data		Byte to send
// @return      none
// *************************************************************************************************
void ps_twi_write(u8 data)
{
	u8 i, mask;
	
	// Set mask byte to 10000000b
	mask = BIT0<<7;
	
	PS_TWI_SDA_OUT;		// SDA is output
	
	for (i=8; i>0; i--)
	{
		PS_TWI_SCL_LO;	// SCL=0
		if ((data & mask) == mask)
		{
			PS_TWI_SDA_HI; // SDA=1
		}
		else 								
		{
			PS_TWI_SDA_LO; // SDA=0 
		}
		mask = mask >> 1;
		twi_delay();
		PS_TWI_SCL_HI;	// SCL=1
		twi_delay();
	}
	
	PS_TWI_SCL_LO;		// SCL=0
	PS_TWI_SDA_IN;		// SDA is input
} 


// *************************************************************************************************
// @fn          ps_twi_read
// @brief       Read bits from SDA
// @param       u8 ack		1=Send ACK after read, 0=Send NACK after read
// @return      u8			Bits read
// *************************************************************************************************
u8 ps_twi_read(u8 ack)
{
	u8 i;
	u8 data = 0;
	
	PS_TWI_SDA_IN;		// SDA is input
	
	for (i=0; i<8; i++)
	{
		PS_TWI_SCL_LO;			// SCL=0
		twi_delay();
		PS_TWI_SCL_HI;			// SCL=0
		twi_delay();
		
		// Shift captured bits to left 
		data = data << 1; 

		// Capture new bit
		if ((PS_TWI_IN & PS_SDA_PIN) == PS_SDA_PIN) data |= BIT0; 
	}

	PS_TWI_SDA_OUT;			// SDA is output
	
	// 1 aditional clock phase to generate master ACK
	PS_TWI_SCL_LO;			// SCL=0
	if (ack == 1)	PS_TWI_SDA_LO		// Send ack -> continue read
	else			PS_TWI_SDA_HI		// Send nack -> stop read
	twi_delay();
	PS_TWI_SCL_HI;			// SCL=0
	twi_delay();
	PS_TWI_SCL_LO;
	
	return (data);
} 



// *************************************************************************************************
// @fn          as_write_register
// @brief  		Write a byte to the pressure sensor
// @param       u8 address			Register address
//				u8 data			Data to write
// @return      u8					
// *************************************************************************************************
u8 ps_write_register(u8 address, u8 data)
{
  volatile u8 success;

  ps_twi_sda(PS_TWI_SEND_START);			// Generate start condition
  
  ps_twi_write((0x11<<1) | PS_TWI_WRITE); 	// Send 7bit device address 0x11 + write bit '0'
  success = ps_twi_sda(PS_TWI_CHECK_ACK);	// Check ACK from device
  if (!success) return (0);
  
  ps_twi_write(address);					// Send 8bit register address
  success = ps_twi_sda(PS_TWI_CHECK_ACK);	// Check ACK from device
  if (!success) return (0);
	
  ps_twi_write(data);						// Send 8bit data to register
  success = ps_twi_sda(PS_TWI_CHECK_ACK);	// Check ACK from device
 // Slave does not send this ACK
 // if (!success) return (0);
  
  ps_twi_sda(PS_TWI_SEND_STOP);				// Generate stop condition

  return (1);
}


// *************************************************************************************************
// @fn          ps_read_register
// @brief       Read a byte from the pressure sensor
// @param       u8 address		Register address
//				u8	mode		PS_TWI_8BIT_ACCESS, PS_TWI_16BIT_ACCESS
// @return      u16			Register content
// *************************************************************************************************
u16 ps_read_register(u8 address, u8 mode)
{
  u8 success;
  u16 data = 0;

  ps_twi_sda(PS_TWI_SEND_START);			// Generate start condition

  ps_twi_write((0x11<<1) | PS_TWI_WRITE); 	// Send 7bit device address 0x11 + write bit '0'
  success = ps_twi_sda(PS_TWI_CHECK_ACK);	// Check ACK from device
  if (!success) return (0);
  
  ps_twi_write(address);					// Send 8bit register address
  success = ps_twi_sda(PS_TWI_CHECK_ACK);	// Check ACK from device
  if (!success) return (0);

  ps_twi_sda(PS_TWI_SEND_RESTART);			// Generate restart condition

  ps_twi_write((0x11<<1) | PS_TWI_READ); 	// Send 7bit device address 0x11 + read bit '1'
  success = ps_twi_sda(PS_TWI_CHECK_ACK);	// Check ACK from device
  if (!success) return (0);
	
  if (mode == PS_TWI_16BIT_ACCESS)
  {
	  data =  ps_twi_read(1) << 8;			// Read MSB 8bit data from register
	  data |= ps_twi_read(0);				// Read LSB 8bit data from register
  }
  else
  {
	  data = ps_twi_read(0);				// Read 8bit data from register
  }  
  
  ps_twi_sda(PS_TWI_SEND_STOP);				// Generate stop condition

  return (data);
}



// *************************************************************************************************
// @fn          ps_get_pa
// @brief       Read out pressure. Format is Pa. Range is 30000 .. 120000 Pa.
// @param       none
// @return      u32		15-bit pressure sensor value (Pa)
// *************************************************************************************************
u32 ps_get_pa(void)
{
	volatile u32 data = 0;
	
	// Get 3 MSB from DATARD8 register
	data = ps_read_register(0x7F, PS_TWI_8BIT_ACCESS);
	data = ((data & 0x07) << 8) << 8;

	// Get 16 LSB from DATARD16 register
	data |= ps_read_register(0x80, PS_TWI_16BIT_ACCESS);
	
	// Convert decimal value to Pa
	data = (data >> 2);
	
	return (data);
}


// *************************************************************************************************
// @fn          ps_get_temp
// @brief       Read out temperature.
// @param       none
// @return      u16		13-bit temperature value in xx.x°K format
// *************************************************************************************************
u16 ps_get_temp(void)
{
	volatile u16 data = 0;
	u16 temp = 0;
	u8 is_negative = 0;
	u16 kelvin;
	
	// Get 13 bit from TEMPOUT register
	data = ps_read_register(0x81, PS_TWI_16BIT_ACCESS);
	
	// Convert negative temperatures
	if ((data & BIT(13)) == BIT(13)) 
	{
		// Sign extend temperature
		data |= 0xC000;
		// Convert two's complement
		data = ~data;
		data += 1;
		is_negative = 1;
	}

	temp = data / 2;
	
	// Convert from °C to °K
	if (is_negative)	kelvin = 2732 - temp;
	else				kelvin = temp + 2732;
	
	return (kelvin);
}

// *************************************************************************************************
// @fn          init_pressure_table
// @brief       Init pressure table with constants
// @param       u32		p 		Pressure (Pa)
// @return      u16				Altitude (m)
// *************************************************************************************************
void init_pressure_table(void)
{
#ifndef FIXEDPOINT
	u8 i;

	for (i=0; i<17; i++) p[i] = p0[i];
#else
	pLast = 101325/4; // Last measured pressure in 4Pa units
	pRef = 101325/4; // Reference pressure at sea level in 4Pa units
	hLast = 0;
#endif
}

#ifdef FIXEDPOINT
// *************************************************************************************************
// @fn          conv_altitude_to_fraction
// @brief       Relative pressure deviation from reference pressure for given altitude estimate.
// @param       s16 hh Altitude estimate (in normalised units).
// @return      Calculated relative pressure deviation for this altitude
// *************************************************************************************************
s16 conv_altitude_to_fraction(s16 hh)
{
	/*
	The fixed part of the function of altitude can be broken into tabulated ranges
	and/or interpolated according to a Taylor series expansion
		 (1 - f) = (1 – h/H0)^b
		         = 1 - h*b/H0 + h^2*b*(b–1)/2/H0^2 – h^3*b8(b–1)*(b-2)/6/H0^3 + …
	At low altitudes h/H0 << 1, so this series tends to converge rapidly and is
	well-suited for fixed point implementation. With one or two additional terms
	the series converges accurately over the range of interest so there is no need
	for table interpolation. For the proposed fixed point implementation we rewrite
	this expression a bit into
		hh = b*h/H0
		(1 - f) = (1 – h/H0)^b
		        = 1 - hh*(1 – hh*(b–1)/2/b*(1 – hh*(b–2)/3/b*(...
	We stick to integer multiply and shift operations. Signed s16 values can contain
	values +/–2^15 and unsigned u16 values 0..2^16. In C multiplication amounts to
	expanding to s32, integer multiply and scaling back by a proper shift operation.

	Given the above equations the natural unit of hh as the first order correction is
	H0/b = 8434.48m. If we accept this as a maximum +/– range we can store s16 hh in
	units of (H0/b)/2^15 = 0,26m which keeps the resolution at less than a foot.
	 */
	s16 f, hf;
	// f  = hh*(b – 4)/5/b, correction relevant above 3.5km:
	// (Could be omitted, but it is relatively little work.)
	f = mult_scale16(hh, 3132);
	// f  = hh*(b – 3)/4/b*(1 - f), correction relevant above 1.3km:
	hf = mult_scale16(hh, 7032);
	f = hf - mult_scale15(hf,f);
	// f = hh*(b – 2)/3/b*(1 - f), correction relevant above 300m:
	hf = mult_scale16(hh, 13533);
	f = hf - mult_scale15(hf,f);
	// f = hh*(b – 1)/2/b*(1 - f), correction relevant above 30m:
	hf = mult_scale16(hh, 26533);
	f = hf - mult_scale15(hf,f);
	// f = hh*(1 - f), the linear part:
	f = hh - mult_scale15(hh,f);
	return f;
}

#endif // FIXEDPOINT


// *************************************************************************************************
// @fn          update_pressure_table
// @brief       Calculate pressure table for reference altitude.
//				Implemented straight from VTI reference code.
// @param       s16		href	Reference height
//				u32		p_meas	Pressure (Pa)
//				u16		t_meas	Temperature (10*°K)
// @return     	none
// *************************************************************************************************
void update_pressure_table(s16 href, u32 p_meas, u16 t_meas)
{
#ifndef FIXEDPOINT
	const float Invt00 = 0.003470415;
	const float coefp  = 0.00006;
	volatile float p_fact; 
	volatile float p_noll;
	volatile float hnoll;
	volatile float h_low = 0;
	volatile float t0;
	u8 i;
	
	// Typecast arguments
	volatile float fl_href 		= href;
	volatile float fl_p_meas 	= (float)p_meas/100;	// Convert from Pa to hPa
	volatile float fl_t_meas	= (float)t_meas/10;		// Convert from 10°K to 1°K

	t0 = fl_t_meas + (0.0065*fl_href);
	
	hnoll  = fl_href/(t0*Invt00);
	
	for (i=0; i<=15; i++)
	{
		if (h0[i] > hnoll) break;
		h_low = h0[i];	
	}
	
	p_noll = (float)(hnoll - h_low)*(1 - (hnoll - (float)h0[i])* coefp)*((float)p0[i] - (float)p0[i-1])/((float)h0[i] - h_low) + (float)p0[i-1];

	// Calculate multiplicator
	p_fact = fl_p_meas/p_noll;
	
	// Apply correction factor to pressure table
	for (i=0; i<=16; i++)
	{
		p[i] = p0[i]*p_fact;	
	}
#else
	// Note: a user-provided sea-level reference pressure in mbar as used by pilots
	// would be straightforward: href = 0; p_meas = (s32)mbar*100;
	// The altitude reading will be iteratively updated.

	// Convert to 4Pa units:
	pLast = (s16)((p_meas+2) >> 2);
	// Convert reference altitude to normalized units:
	if (sys.flag.use_metric_units) { // user_altitude in m
		hLast = 4*href - mult_scale16(href, 7536);
	} else { // user_altitude in ft
		hLast = href + mult_scale16(href,12068);
	}
	s32 f = (s32)0x8000 - conv_altitude_to_fraction(hLast);
	// pRef = p_meas*2^15/f:
	pRef = ((((s32)pLast << 16) + f) >> 1) / f;
	// The long division is acceptable because it happens rarely.
	// The term + f) is for proper rounding.
	// The <<16 and >>1 operations correct for the 15bit scale of f.
#endif
}

#ifndef FIXEDPOINT
// *************************************************************************************************
// @fn          conv_pa_to_meter
// @brief       Convert pressure (Pa) to altitude (m) using a conversion table
//				Implemented straight from VTI reference code.
// @param       u32		p_meas	Pressure (Pa)
//				u16		t_meas	Temperature (10*°K)
// @return      s16				Altitude (m)
// *************************************************************************************************
s16 conv_pa_to_meter(u32 p_meas, u16 t_meas)
{
	const float coef2  = 0.0007;
	const float Invt00 = 0.003470415;
	volatile float hnoll;
	volatile float t0;
	volatile float p_low;
	volatile float fl_h;
	volatile s16 h;
	u8 i;

	// Typecast arguments
	volatile float fl_p_meas = (float)p_meas/100;	// Convert from Pa to hPa
	volatile float fl_t_meas = (float)t_meas/10;		// Convert from 10°K to 1°K
	
	for (i=0; i<=16; i++)
	{
		if (p[i] < fl_p_meas) break;
		p_low = p[i];
	}
		
	if (i==0) 
	{
		hnoll = (float)(fl_p_meas - p[0])/(p[1] - p[0])*((float)(h0[1] - h0[0]));
	}
	else if (i<15) 
	{
		hnoll = (float)(fl_p_meas - p_low)*(1 - (fl_p_meas - p[i])* coef2)/(p[i] - p_low)*((float)(h0[i] - h0[i-1])) + h0[i-1];
	}
	else if (i==15)
	{
		hnoll = (float)(fl_p_meas - p_low)/(p[i] - p_low)*((float)(h0[i] - h0[i-1])) + h0[i-1];
	}
	else // i==16
	{
		hnoll = (float)(fl_p_meas - p[16])/(p[16] - p[15])*((float)(h0[16] - h0[15])) + h0[16];
	}
	
	// Compensate temperature error
	t0 = fl_t_meas/(1 - hnoll*Invt00*0.0065);
	fl_h = Invt00*t0*hnoll;
	h = (u16)fl_h;
	
	return (h);
}
#else
// *************************************************************************************************
// @fn          conv_pressure_to_altitude
// @brief       Calculates altitude from current pressure, and
//				stored reference pressure at sea level and previous altitude estimate.
//				Temperature info is ignored.
// @param       u32		p_meas	Pressure (Pa)
// @param		u16		t_meas	Temperature (10*°K) Ignored !!!
// @return      Estimated altitude in user-selected unit (m or ft)
//              (internally filtered, slightly sluggish).
// *************************************************************************************************
s16 conv_pa_to_altitude(u32 p_meas, u16 t_meas)
{
	/*
	Assumption: fixed, linear T(h)
	T = T0 – dTdh*h
	with
	T0 = 288.15K (15C)
	dTdh = 6.5mK/m

	Basic differential equation:
		dh = -(R/G)*T(H)*dp/p
	Solution:
		H = H0*(1 – (p/pRef)^a)
	with
		H0 = T0/dTdh = 44330.77m
		pRef = adjustable reference pressure at sea level (h=0).
		a = dTdH*R/G = 0.190263
		R = 287.052m^2/s^2/K
		G = 9.80665 (at medium latitude)

	We assume T0 and the temperature profile to be fixed; the temperature reading
	of the watch is not very useful since it is strongly influenced by body heat,
	clothing, shelter, etc.

	Straight evaluation of h(p) requires an unattractive long division p/pRef
	with pRef the adjustable reference pressure, and the Taylor expansion does
	not converge very quickly.

	Evaluation of p(h) requires a more attractive multiplication by the
	user-adjustable reference pressure pRef:
		f =(1 – h/H0)^b
		p = pRef*f
	with
		b = 1/a = G/(dTdH*R) = 5.255896
	In a very crude linear iteration the h value can be updated by
		delta_h = –delta_p / dpdh
	The slope dpdh varies by about a factor two over the range of interest,
	but we can pick a fixed value on the safe side and accept that the updates
	are a bit more damped at higher altitudes.

	The sensor provides 19bit absolute pressure in units of 0.25Pa, but that is more
	resolution than we can easily handle in the multiplications. We store measured
	pressure p, reference pressure pRef and calculated pressure as u16 in units of 4Pa.

	In the units chosen for p (4Pa) and for hLast (see function conv_altitude_to_fraction),
	the slope dpdh is about -0.75 at sea level down to -0.375 at high altitudes. To avoid
	overshoot and instabilities we assume a bigger value and accept a minor amount of
	extra filtering delay at higher altitudes. The factor 1/0.75 is approximated by 1.
	*/
	// Scale to 4Pa units:
	s16 p = (s16)((p_meas+2) >> 2);
	// Predictor to speed up response to pressure changes:
//	hLast -= p - pLast; // Factor of about 1/0.75 would be better.
	// Store current pressure for next predictor:
	pLast = p;
	// Calculate pressure ratio based on guessed altitude (serious DSP work):
	s16 f = conv_altitude_to_fraction(hLast);
	// Calculate pressure expected for guessed height
	u16 pCalculated = pRef - mult_scale15(pRef,f);
	// This calculation is correct within about 7Pa.
	// We still have to reverse the solution with a linearly improved guess:
	hLast -= p - pCalculated;
	// Iteration gain factor of about 1/0.75 would result in faster convergence,
	// but even the big initial jump when the altimeter is switched on converges
	// in some 5 or 6 steps to about 1m accuracy.

	if (sys.flag.use_metric_units) {
		// Altitude in meters (correct within about 0.7m):
		return mult_scale16(hLast, 16869);
	} else {
		// Altitude in feet (correct within 1.5ft):
		return mult_scale15(hLast, 27672);
	}
}
#endif // FIXEDPOINT
