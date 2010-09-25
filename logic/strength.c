/* TODO add copyright header */
// *************************************************************************************************
// Include section

// system
#include "project.h"

// driver
#include "display.h"

// logic
#include "menu.h"
#include "strength.h"

/* TODO add display function */
void display_strength_time(u8 line, u8 update) 
{
  display_chars(LCD_SEG_L1_2_0, (u8*)"WUFF", SEG_ON);
}
