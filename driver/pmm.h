//====================================================================
//    File: PMM.h
//
//    This file is used in conjunction with PMM.c to set the core
//    voltage level of a device. To set a core voltage level, call 
//    SetVCore(level). See RF project(s) for example usage. 
// 
//    Version 1.0 first 
//    07/14/07
//
//====================================================================


#ifndef __PMM
#define __PMM


//====================================================================
/**
  * Set the VCore to a new level
  *
  * \param level       PMM level ID
  */
void SetVCore (unsigned char level);

//====================================================================
/**
  * Set the VCore to a new higher level
  *
  * \param level       PMM level ID
  */
void SetVCoreUp (unsigned char level);

//====================================================================
/**
  * Set the VCore to a new Lower level
  *
  * \param level       PMM level ID
  */
void SetVCoreDown (unsigned char level);

#endif /* __PMM */
