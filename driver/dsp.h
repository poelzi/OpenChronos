/*
 * dsp.h
 *
 *  Created on: Aug 5, 2010
 *      Author: nlv13513
 */

// *************************************************************************************************
#ifndef DSP_H_
#define DSP_H_

// Include section
#include "project.h"

// *************************************************************************************************
// Prototypes section
extern s16 mult_scale16(s16 a, s16 b); // returns (s16)((s32)a*b + 0x8000) >> 16
extern s16 mult_scale15(s16 a, s16 b); // returns (s16)(((s32)a*b << 1) + 0x8000) >> 16

#endif /*DSP_H_*/
