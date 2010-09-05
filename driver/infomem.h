/****
 * written by Lukas Middendorf
 * 
 * use as desired but do not remove this notice
 */

#include "project.h"

#ifndef INFOMEM_H_
#define INFOMEM_H_

/*
 * This driver allows applications to store data in the information memory flash 
 * without interfering with each other (except for the total available memory) or
 * having to care about the characteristics of flash memory.
 * 
 * infomem_ready() has to be called before any other function does work (except
 * infomem_init, but use infomem_ready to check if infomem_init is really needed).
 * 
 * Single applications should only use the infomem_app_*() functions ary infomem_space(),
 * the rest of the functions should be used only in the global part of the firmware
 * (or in a dedicated application which's function it is to do memory maintenance tasks).
 * 
 * Use infomem_app_replace() with count>0 to initialize memory for application. This has
 * to be redone if data of size zero was present and the application header was therefore
 * also removed (replace with zero count, clear, delete with zero offset or modify with 
 * zero count and offset).
 * 
 * All pointers and addresses have to be word addresses (even numbers) and all counts
 * are given in units of words (two bytes).
 */


//check if infomem is initialized and in sane state, return amount of data present
extern s16 infomem_ready();
//write infomem data structure
extern s16 infomem_init(u16 start, u16 end);
//return amount of free space
extern s16 infomem_space();
//change start and end address of data storage (can change size)
extern s16 infomem_relocate(u16 start, u16 end);
//delete complete data storage (only managed space)
extern s16 infomem_delete_all(void);

//return how much data for the application is available
extern s16 infomem_app_amount(u8 identifier);
//read count bytes of data with offset for given application into prepared memory
extern s16 infomem_app_read(u8 identifier, u16* data, u8 count, u8 offset);
//replace all memory content for application by new data
extern s16 infomem_app_replace(u8 identifier, u16* data, u8 count);
//delete all memory content for application
extern s16 infomem_app_clear(u8 identifier);
//delete all memory content beginning with offset
extern s16 infomem_app_delete(u8 identifier,u8 offset);
//modify given bytes of data
extern s16 infomem_app_modify(u8 identifier, u16* data, u8 count, u8 offset);



struct infomem
{
	u16*		startaddr; //starting address (position of header)
	u8			size;  //size of payload in words
	u8			maxsize;  //maximum size of payload in words
	volatile u8	not_lock;  //memory is not locked for write
	u8			sane;  //sanity check passed
};
// extern struct infomem sInfomem;


#define INFOMEM_IDENTIFIER 0x5a74
#define INFOMEM_TERMINATOR 0xdaf4
#define INFOMEM_SANE 0xda

#define INFOMEM_START 0x1800
#define INFOMEM_D 0x1800
#define INFOMEM_C 0x1880
#define INFOMEM_B 0x1900
#define INFOMEM_A 0x1980
#define INFOMEM_SEGMENT_SIZE 128
#define INFOMEM_SEGMENT_WORDS INFOMEM_SEGMENT_SIZE/2
#define INFOMEM_ERASED_WORD 0xFFFF


#endif /*INFOMEM_H_*/