//****************************************************************************//
// Function Library for setting the PMM
//
//  This file is used in conjunction with PMM.c to set the core
//  voltage level of a device. To set a core voltage level, call 
//  SetVCore(level). See RF project(s) for example usage. 
//
//  Original programm                                           Stefan Schauer
//  Rev 1.1: changed VCoreUp to fit with recommended flow (09/04/2008)
// 
//****************************************************************************//
#include "cc430x613x.h"
#include "pmm.h"


//****************************************************************************//
// Set VCore level
// SetVCore level is called from the user API 
//****************************************************************************//
void SetVCore (unsigned char level)        // Note: change level by one step only
{
  unsigned char actLevel;
  do {
    actLevel = PMMCTL0_L & PMMCOREV_3;
    if (actLevel < level) 
      SetVCoreUp(++actLevel);               // Set VCore (step by step)
    if (actLevel > level) 
      SetVCoreDown(--actLevel);             // Set VCore (step by step)
  }while (actLevel != level);
}


//****************************************************************************//
// Set VCore up 
//****************************************************************************//
void SetVCoreUp (unsigned char level)        // Note: change level by one step only
{
  PMMCTL0_H = 0xA5;                         // Open PMM module registers for write access
 
  SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;     // Set SVS/M high side to new level

  SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;     // Set SVM new Level    
  while ((PMMIFG & SVSMLDLYIFG) == 0);      // Wait till SVM is settled (Delay)
  PMMCTL0_L = PMMCOREV0 * level;            // Set VCore to x
  PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);        // Clear already set flags
  if ((PMMIFG & SVMLIFG))
    while ((PMMIFG & SVMLVLRIFG) == 0);     // Wait till level is reached
 
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;     // Set SVS/M Low side to new level
  PMMCTL0_H = 0x00;                         // Lock PMM module registers for write access
}



//****************************************************************************//
// Set VCore down 
//****************************************************************************//
void SetVCoreDown (unsigned char level)
{
  PMMCTL0_H = 0xA5;                         // Open PMM module registers for write access
  SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;     // Set SVS/M Low side to new level
  while ((PMMIFG & SVSMLDLYIFG) == 0);      // Wait till SVM is settled (Delay)
  PMMCTL0_L = (level * PMMCOREV0);          // Set VCore to 1.85 V for Max Speed.
  PMMCTL0_H = 0x00;                         // Lock PMM module registers for write access
}


//****************************************************************************//
