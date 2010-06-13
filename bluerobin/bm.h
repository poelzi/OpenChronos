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
// Standard definitions, have to be included in every source and header file.
// *************************************************************************************************

#ifndef __BM_H
#define __BM_H


#if (defined __IAR_SYSTEMS_ASM) || (defined __IAR_SYSTEMS_ASM__)
  #define _ASSEMBLER_USED_
#endif

#ifndef _ASSEMBLER_USED_
  #include <stddef.h>
#endif

#ifndef FALSE
  // the classic false
  #define FALSE (0 == 1)
#endif

#ifndef TRUE
  // the classic true
  #define TRUE  (1 == 1)
#endif

#ifndef USE_RAW_ATTR
  // per default this feature is disabled
  #define USE_RAW_ATTR FALSE
#endif

// *************************************************************************************************
// First Section: Basic Data Types
// *************************************************************************************************

// Fundamental #definitions
// CPU target idents are used for target dependent compilations

// Texas Instruments MSP430
#define _TI_MSP430_  (16)


// Find the currently running compiler
// and make the related #define's

// _IAR_TID_                target ID from IAR compilers
// _CPU_TID_                remap to enum of processor target numbers
// _CPU_8BIT_INT_           type for 8 bit int
// _CPU_16BIT_INT_          type for 16 bit int
// _CPU_32BIT_INT_          type for 32 bit int
// _CPU_32BIT_FLOAT_        type for 32 bit float
// _CPU_64BIT_FLOAT_        type for 64 bit float
// INTERRUPT                declares an interrupt service routine without an entry in the vector table
// ISR(vector)              declares an interrupt service routine which is added in vector table at offset vector
// MONITOR                  declares a function atomic
// INTERRUPTS_ENABLE        remap to the intrinsic for enable interrupts
// INTERRUPTS_DISABLE       remap to the intrinsic for disable interrupts
// NO_OPERATION             remap to the intrinsic for no operation
// _CPU_DIRECTION_OUT_1_    if TRUE the direction register indicates with an 1: direction is output
// _CPU_EDGE_HIGH_LOW_1_    if TRUE the edge select register indicates with an 1: trigger on high low
// NO_INIT                  declare a variable as not initialized
// INLINE_FUNC              declare a function as inline for release builds


#if ((defined __IAR_SYSTEMS_ICC) || (defined __IAR_SYSTEMS_ASM)) && (__IAR_SYSTEMS_ICC__ < 2)
  // Found IAR Compiler with classic IAR frontend
  #ifndef _IAR_TID_
    #define _IAR_TID_ ((__TID__ >> 8) & 0x7f)
  #endif

  #define INTERRUPT   interrupt
  #define ISR(vector) interrupt [(vector)]
  #define MONITOR     monitor

  #if ((_IAR_TID_) == 43)
    // Found Texas Instruments MSP430 CPU
    #define _CPU_TID_             _TI_MSP430_
    #define _CPU_DIRECTION_OUT_1_ TRUE
    #define _CPU_EDGE_HIGH_LOW_1_ TRUE
    #define INTERRUPTS_ENABLE()   _EINT()
    // WA for HW bug, add a NOP
    #define INTERRUPTS_DISABLE()  { _DINT(); _NOP(); }
    #define NO_OPERATION()        _NOP()

  #else
    #error "Unknown IAR Compiler, the file bm.h has to be expanded !"
  #endif

#elif defined __IAR_SYSTEMS_ICC__
  // Found IAR Compiler with EDG frontend
  #define _IAR_TID_ ((__TID__ >> 8) & 0x7f)

  #if USE_RAW_ATTR == TRUE
    // Use the raw attribute in ISR's
    #define _RAW __raw
  #else
    // Empty define RAW as it is not used
    #define _RAW
  #endif
  #define INTERRUPT   _RAW __interrupt
  #define MONITOR     __monitor
  #define NO_INIT     __no_init
  #define INTERRUPTS_ENABLE()   __enable_interrupt()
  #define INTERRUPTS_DISABLE()  __disable_interrupt()
  #define NO_OPERATION()        __no_operation()

  #ifndef DEBUG
    // Force inlining of function in release builds
    #define INLINE_FUNC PRAGMA(inline=forced)
  #else
    // Do not force inlining of function in debug builds
    #define INLINE_FUNC
  #endif

  #if (!defined CODECHECK) && (!defined __DA_C__)
    // Define to a new way of using #pragmas in preprocessor
    #define PRAGMA(x) _Pragma(#x)
    #define ISR(x) PRAGMA(vector = (x)) INTERRUPT
  #endif

  #if ((_IAR_TID_) == 43)
    // Found Texas Instruments MSP430 CPU  (V2)
    #define _CPU_TID_             _TI_MSP430_
    #define _CPU_DIRECTION_OUT_1_ TRUE
    #define _CPU_EDGE_HIGH_LOW_1_ TRUE
    #define _CPU_64BIT_INT_       long long
    #if __VER__ < 220
      // WA for HW bug, add a nop after DINT, Compiler has a bugfix since version 2.20
      #undef  INTERRUPTS_DISABLE
      #define INTERRUPTS_DISABLE()  { __disable_interrupt(); __no_operation(); }
    #endif

  #else
    #error "Unknown new IAR Compiler, the file bm.h has to be expanded !"
  #endif


#elif defined __CCE__
  // Found CCE Compiler 
  #define INTERRUPT   __interrupt
  #define MONITOR    // __monitor
  #define NO_INIT     __no_init
  #define INTERRUPTS_ENABLE()   __enable_interrupt()
  #define INTERRUPTS_DISABLE()  __disable_interrupt()
  #define NO_OPERATION()        __no_operation()

  #ifndef DEBUG
    // Force inlining of function in release builds
    #define INLINE_FUNC PRAGMA(inline=forced)
  #else
    // Do not force inlining of function in debug builds
    #define INLINE_FUNC
  #endif

	// Found Texas Instruments MSP430 CPU  (V2)
	#define _CPU_TID_             _TI_MSP430_
	#define _CPU_DIRECTION_OUT_1_ TRUE
	#define _CPU_EDGE_HIGH_LOW_1_ TRUE
	//#define _CPU_64BIT_INT_       long long

//  #endif

// PFS added
#elif defined __MSP430__
  // Found MSP430 Compiler 
  // PFS #define INTERRUPT   interrupt
  #define ISR(vector) interrupt(vector) 
  #define MONITOR    // __monitor
  #define NO_INIT    _reset_vector__ // PFS removed __no_init
  #define INTERRUPTS_ENABLE()   _EINT() //PFS or eint()
  #define INTERRUPTS_DISABLE()  _DINT() // PFS or dint()
  #define NO_OPERATION()       // PFS __no_operation()

  #ifndef DEBUG
    // Force inlining of function in release builds
    #define INLINE_FUNC PRAGMA(inline=forced)
  #else
    // Do not force inlining of function in debug builds
    #define INLINE_FUNC
  #endif

	// Found Texas Instruments MSP430 CPU  (V2)
	#define _CPU_TID_             _TI_MSP430_
	#define _CPU_DIRECTION_OUT_1_ TRUE
	#define _CPU_EDGE_HIGH_LOW_1_ TRUE
	//#define _CPU_64BIT_INT_       long long

//  #endif


#else
  #error "Unknown Compiler, the file bm.h has to be expanded !"
#endif


#ifndef _ASSEMBLER_USED_
  // Get the limits to autodetect the size of integral types
  #include <limits.h>
  // Get floats to autodetect the size of float types
  #include <float.h>

  // ***********************************************************************************************
  //
  // Common basic data types
  //
  // ***********************************************************************************************
  #if UCHAR_MAX == 0xFFu
    #define _CPU_8BIT_INT_ char
  #else
    #error "unable to get size of u8 automatically"
  #endif

  #if USHRT_MAX == 0xFFFFu
    #define _CPU_16BIT_INT_ short
  #elif UINT_MAX == 0xFFFFu
    #define _CPU_16BIT_INT_ int
  #else
    #error "unable to get size of u16 automatically"
  #endif

  #if USHRT_MAX == 0xFFFFFFFFu
    #define _CPU_32BIT_INT_ short
  #elif UINT_MAX == 0xFFFFFFFFu
    #define _CPU_32BIT_INT_ int
  #elif ULONG_MAX == 0xFFFFFFFFu
    #define _CPU_32BIT_INT_ long
  #else
    #error "unable to get size of u32 automatically"
  #endif

  #ifdef __IAR_SYSTEMS_ICC__
    #if __IAR_SYSTEMS_ICC__ > 1
      #define _CPU_32BIT_FLOAT_ float
      #if __DOUBLE_SIZE__ == 8
        #define _CPU_64BIT_FLOAT_ double
      #endif
    #endif
  #endif

  #ifndef _CPU_32BIT_FLOAT_
    #if FLT_MANT_DIG == 24
      #define _CPU_32BIT_FLOAT_ float
    #elif DBL_MANT_DIG == 24
      #define _CPU_32BIT_FLOAT_ double
    #else
      #error "unable to get size of f32 automatically"
    #endif

    #if DBL_MANT_DIG == 53
      #define _CPU_64BIT_FLOAT_ double
    #endif
  #endif


  // ***********************************************************************************************
  //
  // Following lines #typedef the basic data types in a compiler independent way.
  //
  // ***********************************************************************************************

  #ifdef _CPU_8BIT_INT_
    // unsigned 8 bit
    typedef unsigned _CPU_8BIT_INT_ u8 ;
    // unsigned 8 bit max value
    #define U8_MAX (0xFFU)
    // signed 8 bit max value
    typedef   signed _CPU_8BIT_INT_ s8 ;
    // signed 8 bit min value
    #define S8_MIN (-127 - 1)
    // signed 8 bit max value
    #define S8_MAX (127)
  #endif

  #ifdef _CPU_16BIT_INT_
    // unsigned 16 bit
    typedef unsigned _CPU_16BIT_INT_ u16 ;
    // unsigned 16 bit max value
    #define U16_MAX (0xFFFFU)
    // signed 16 bit
    typedef   signed _CPU_16BIT_INT_ s16 ;
    // signed 16 bit min value
    #define S16_MIN (-32767 - 1)
    // signed 16 bit max value
    #define S16_MAX (32767)
  #endif

  #ifdef _CPU_32BIT_INT_
    // unsigned 32 bit
    typedef unsigned _CPU_32BIT_INT_ u32 ;
    // unsigned 32 bit max value
    #define U32_MAX (0xFFFFFFFFUL)
    // signed 32 bit
    typedef   signed _CPU_32BIT_INT_ s32 ;
    // signed 32 bit min value
    #define S32_MIN (-2147483647L - 1L)
    // signed 32 bit max value
    #define S32_MAX (2147483647L)
  #endif

  #ifdef _CPU_64BIT_INT_
    // unsigned 64 bit
    typedef unsigned _CPU_64BIT_INT_ u64 ;
    // signed 64 bit
    typedef   signed _CPU_64BIT_INT_ s64 ;
  #endif

  #ifdef _CPU_32BIT_FLOAT_
    // float 32 bit
    typedef _CPU_32BIT_FLOAT_ f32 ;
    // number of digits in mantissa of f32
    #define F32_MANT_DIG   (24)
    // epsilon for f32
    #define F32_EPSILON    (1.192092896e-07)
    // number of digits of precision of f32
    #define F32_DIG        (6)
    // exponent min of f32
    #define F32_MIN_EXP    (-125)
    // min positive value of f32
    #define F32_MIN        (1.175494351e-38)
    // decimal exponent min of f32
    #define F32_MIN_10_EXP (-37)
    // exponent max of f32
    #define F32_MAX_EXP    (128)
    // max value of f32
    #define F32_MAX        (3.402823466e+38)
    // decimal exponent max of f32
    #define F32_MAX_10_EXP (38)
  #endif

  #ifdef _CPU_64BIT_FLOAT_
    // float 64 bit
    typedef _CPU_64BIT_FLOAT_ f64 ;
    // number of digits in mantissa of f64
    #define F64_MANT_DIG   (53)
    // epsilon for f64
    #define F64_EPSILON    (2.2204460492503131e-016)
    // number of digits of precision of f64
    #define F64_DIG        (15)
    // exponent min of f64
    #define F64_MIN_EXP    (-1021)
    // min positive value of f64
    #define F64_MIN        (2.2250738585072014e-308)
    // decimal exponent min of f64
    #define F64_MIN_10_EXP (-307)
    // exponent max of f64
    #define F64_MAX_EXP    (1024)
    // max value of f64
    #define F64_MAX        (1.7976931348623158e+308)
    // decimal exponent max of f64
    #define F64_MAX_10_EXP (308)
  #endif
    
  typedef unsigned char BYTE;
  typedef unsigned int  WORD;
  typedef unsigned long DWORD;
    
#endif // _ASSMEBLER_USED_

// A macro that calculates the number of bits of a given type
#ifndef BITSIZEOF
#define BITSIZEOF(type) (sizeof(type) * CHAR_BIT)
#endif

/* A macro that generates a bit mask according to a given bit number.
 * Example:
 * - BIT(0) expands to 1 (== 0x01)
 * - BIT(3) expands to 8 (== 0x08)
 */
#define BIT(x) (1uL << (x))

/* A macro that generates a bit mask according to a given bit number with a cast to type.
 * The difference to BIT(X) is the additional type argument T that is used to cast the type of the
 * constant 1 and the type of the result as well.
 * Example:
 * - BIT_T(u8,0)  expands to (u8)1  (== 0x01)
 * - BIT_T(u32,3) expands to (u32)8 (== 0x08L)
 */
#define BIT_T(t, x) ((t)((t)1 << (x)))

/* A macro to calculate the position of the highest bit.
 * The result is same as LOG2 in case only one bit is set. Use with constant values only because
 * of code size.
 */
#define BIT_HIGHEST(Input_u32) (     \
  ( (Input_u32) & BIT(31) ) ? 31 : ( \
  ( (Input_u32) & BIT(30) ) ? 30 : ( \
  ( (Input_u32) & BIT(29) ) ? 29 : ( \
  ( (Input_u32) & BIT(28) ) ? 28 : ( \
  ( (Input_u32) & BIT(27) ) ? 27 : ( \
  ( (Input_u32) & BIT(26) ) ? 26 : ( \
  ( (Input_u32) & BIT(25) ) ? 25 : ( \
  ( (Input_u32) & BIT(24) ) ? 24 : ( \
  ( (Input_u32) & BIT(23) ) ? 23 : ( \
  ( (Input_u32) & BIT(22) ) ? 22 : ( \
  ( (Input_u32) & BIT(21) ) ? 21 : ( \
  ( (Input_u32) & BIT(20) ) ? 20 : ( \
  ( (Input_u32) & BIT(19) ) ? 19 : ( \
  ( (Input_u32) & BIT(18) ) ? 18 : ( \
  ( (Input_u32) & BIT(17) ) ? 17 : ( \
  ( (Input_u32) & BIT(16) ) ? 16 : ( \
  ( (Input_u32) & BIT(15) ) ? 15 : ( \
  ( (Input_u32) & BIT(14) ) ? 14 : ( \
  ( (Input_u32) & BIT(13) ) ? 13 : ( \
  ( (Input_u32) & BIT(12) ) ? 12 : ( \
  ( (Input_u32) & BIT(11) ) ? 11 : ( \
  ( (Input_u32) & BIT(10) ) ? 10 : ( \
  ( (Input_u32) & BIT( 9) ) ?  9 : ( \
  ( (Input_u32) & BIT( 8) ) ?  8 : ( \
  ( (Input_u32) & BIT( 7) ) ?  7 : ( \
  ( (Input_u32) & BIT( 6) ) ?  6 : ( \
  ( (Input_u32) & BIT( 5) ) ?  5 : ( \
  ( (Input_u32) & BIT( 4) ) ?  4 : ( \
  ( (Input_u32) & BIT( 3) ) ?  3 : ( \
  ( (Input_u32) & BIT( 2) ) ?  2 : ( \
  ( (Input_u32) & BIT( 1) ) ?  1 : ( \
  ( (Input_u32) & BIT( 0) ) ?  0 :  -1uL ))))))))))))))))))))))))))))))))


#ifndef MONITOR
  #define MONITOR
#endif

#ifndef NO_INIT
  #define NO_INIT
#endif

#ifndef INTERRUPT
  #define INTERRUPT
#endif

#ifndef ISR
  #define ISR(ignore)
#endif

#ifndef INLINE_FUNC
  #define INLINE_FUNC
#endif

#ifndef INTERRUPTS_ENABLE
  #define INTERRUPTS_ENABLE()
#endif

#ifndef INTERRUPTS_DISABLE
  #define INTERRUPTS_DISABLE()
#endif


#endif // __BM_H
