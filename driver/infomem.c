/****
 * written by Lukas Middendorf
 * 
 * use as desired but do not remove this notice
 */


#include "project.h"

#ifdef CONFIG_INFOMEM

#include "infomem.h"

struct infomem sInfomem;

void infomem_write_flash_segment(u16* start, u16* data, u8 erase);
void infomem_insert_delete_modify(u16* start, u16* data, u8 del_count, u8 ins_count, u16** mod_addr, u16* mod_data, u8 mod_count, u16* free_start, u16* free_stop);
void infomem_write_data(u16* start, u16* data, u8 count);
u16* infomem_get_app_addr(u8 identifier);

#define infomem_waitbusy() \
	while(1) \
	{ \
		if(!(FCTL3 & BUSY)) \
			break; \
	}

// write one complete flash segment
//        FOR INTERNAL USE ONLY
//
// erases segment when needed. Only writes to a long word if data has changed
//data has size of at least 128 Byte
//erase 0: do not erase; 1: test if need to erase; 2: erase without test
void infomem_write_flash_segment(u16* start, u16* data, u8 erase)
{
	int i;
	
	//check if we need to erase
	if(erase ==1)
	{
		for(i=0; i <INFOMEM_SEGMENT_WORDS; i++)
		{
			//we have to erase if the new data has bits 1 that are 0 already
			if( ( start[i] | data[i] ) != start[i])
			{
				erase=2;
				break;
			}
		}
	}
	
	#ifdef USE_WATCHDOG
	//hold watch dog timer
	WDTCTL = (WDTCTL &0xff) | WDTPW | WDTHOLD;
	#endif
	
	infomem_waitbusy()
	
	//remove LOCK and LOCKA bit if needed (LOCKA is toggled if it is written as 1)
	if(start == (u16*)INFOMEM_A && (FCTL3 & LOCKA))
	{
		FCTL3 = FWKEY | LOCKA;
	}
	else
	{
		FCTL3 = FWKEY;
	}
	
	//remove LOCKINFO bit
	FCTL4 = FWKEY ;
	
	if(erase==2)
	{
	FCTL1 = FWKEY | ERASE;
	*start = 0;
	infomem_waitbusy()
	}
	
	//set long-word write mode
	FCTL1 = FWKEY | BLKWRT;
	
	//write long words if the new data is different from the old
	for(i=0; i< INFOMEM_SEGMENT_WORDS; i+=2)
	{
		if(start[i] != data[i] || start[i+1] != data[i+1] )
		{
			start[i  ] = data[i  ];
			start[i+1] = data[i+1];
			infomem_waitbusy()
		}
	}
	
	//leave write mode
	FCTL1 = FWKEY;
	//set LOCKINFO bit
	FCTL4 = FWKEY | (FCTL4 & 0xff) | LOCKINFO;
	
	//set LOCK bit
	FCTL3 = FWKEY | (FCTL3 & 0xff) | LOCK;
	
	#ifdef USE_WATCHDOG
	//restart and reset watchdog timer
	WDTCTL = (WDTCTL &0xff & ~WDTHOLD ) | WDTPW | WDTCNTCL;
	#endif
}


//write count words at address start
void infomem_write_data(u16* start, u16* data, u8 count)
{
	infomem_insert_delete_modify(start, data, count, count, NULL, NULL, 0, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
}

// function to do nearly anything with memory
//
//        FOR INTERNAL USE ONLY
//
// mod_addr[n] < mod_addr[n+1]
// the mod always precedes data (data is not read in that case and can point to invalid memory location)
// mod_addr<free_start+ins_count-del_count
// when data is NULL and ins_count>0, INFOMEM_ERASED_WORD will be inserted instead
// memory right of free_stop is never changed
// free_start is the first address after the meaningful data (BEFORE any insert or delete)
// mod_addr are addresses AFTER the insert/delete
void infomem_insert_delete_modify(u16* start, u16* data, u8 del_count, u8 ins_count, u16** mod_addr, u16* mod_data, u8 mod_count, u16* free_start, u16* free_stop)
{
	int i;
	u8 next_mod;
	int more=ins_count-del_count;
	
	//beginning of segment that is the first (address wise) to be modified
	u16* segment_first;
	//beginning of the segment that is the last (address wise) to be modified
	u16* segment_last;
	
	//find first modified flash segment
	if((mod_count>0) && (mod_addr[0]< start))
	{
		segment_first=(u16*)((u16) mod_addr[0] & ~(INFOMEM_SEGMENT_SIZE-1));
	}
	else
	{
		segment_first=(u16*)((u16) start & ~(INFOMEM_SEGMENT_SIZE-1));
	}
	
	//find last modified flash segment
	if(more==0)
	{
		if((mod_count>0) && (mod_addr[mod_count-1]>=start+ins_count))
		{
			segment_last= (u16*)((u16) mod_addr[mod_count-1] & ~(INFOMEM_SEGMENT_SIZE-1));
		}
		else
		{
			segment_last= (u16*)((u16) (start+ins_count-1) & ~(INFOMEM_SEGMENT_SIZE-1));
		}
	}
	else if(more>0)
	{
		segment_last= (u16*)((u16) (free_start+more-1) & ~(INFOMEM_SEGMENT_SIZE-1));
	}
	else  //more<0
	{
		segment_last= (u16*)((u16) (free_start-1) & ~(INFOMEM_SEGMENT_SIZE-1));
	}
	
	//we need buffer memory to store a flash page while it is erased and rewritten
	u16 buf[INFOMEM_SEGMENT_WORDS];
	
	//position of the newly to insert data relative to the beginning of the segment
	int data_offset;
	
	//we insert data, so start with the last segment and make your way to the beginning
	if(more>0)
	{
		//start with the last mod
		next_mod=mod_count;
		//while we have not processed all modified flash segments
		while(segment_first<=segment_last)
		{
			data_offset=((u16)start - (u16)segment_last)/2;
			
			for(i=INFOMEM_SEGMENT_WORDS-1;i>=0;i--)
			{
				//data we need to preserve
				if( (segment_last+i) >= free_stop)
				{
					buf[i]=segment_last[i];
				}
				//data to change with single modify
				else if( next_mod!=0 && (segment_last+i) == mod_addr[next_mod-1] )
				{
					buf[i] = mod_data[next_mod-1];
					next_mod --;
				}
				//data we do not care about (leave erased)
				else if( (segment_last+i) >= (free_start+more))
				{
					buf[i]=INFOMEM_ERASED_WORD;
				}
				//data not to change
				else if((segment_last+i) < start)
				{
					buf[i]=segment_last[i];
				}
				//data following the data we insert
				else if( (segment_last+i) >= (start+ins_count))
				{
					buf[i]=segment_last[i-more];
				}
				//data to insert
				else // if( (segment_last+i) >= start &&  (segment_last+i) < (start+ins_count) )
				{
					if(data == NULL)
					{
						buf[i]=INFOMEM_ERASED_WORD;
					}
					else
					{
						buf[i]=data[i-data_offset];
					}
				}
			}
			//write buffer to flash
			infomem_write_flash_segment(segment_last,buf,1);
			//we have processed the last segment, process the next in the next loop run
			segment_last-=INFOMEM_SEGMENT_WORDS;
		}
	}
	//start with the first segment
	else //if more<0
	{
		//start with the first mod
		next_mod=0;
		while(segment_first<=segment_last)
		{
			data_offset=((u16)start - (u16)segment_first)/2;
			
			for(i=0;i<INFOMEM_SEGMENT_WORDS;i++)
			{
				//data we need to preserve
				if( (segment_first+i) >= free_stop)
				{
					buf[i]=segment_first[i];
				}
				//data to change with single modify
				else if( next_mod!=mod_count && (segment_first+i) == mod_addr[next_mod] )
				{
					buf[i] = mod_data[next_mod];
					next_mod++;
				}
				//data we do not care about (leave erased)
				else if( (segment_first+i) >= (free_start+more))
				{
					buf[i]=INFOMEM_ERASED_WORD;
				}
				//data not to change
				else if((segment_first+i) < start)
				{
					buf[i]=segment_first[i];
				}
				//data following the data we insert
				else if( (segment_first+i) >= (start+ins_count))
				{
					buf[i]=segment_first[i-more];
				}
				//data to insert
				else // if( (segment_first+i) >= start &&  (segment_first+i) < (start+ins_count) )
				{
					if(data == NULL)
					{
						buf[i]=INFOMEM_ERASED_WORD;
					}
					else
					{
						buf[i]=data[i-data_offset];
					}
				}
			}
			infomem_write_flash_segment(segment_first,buf,1);
			segment_first +=INFOMEM_SEGMENT_WORDS;
		}
	}
}

// *************************************************************************************************
// @fn          infomem_get_app_addr
// @brief       return the address of the header for an address
//				FOR INTERNAL USE ONLY
// @param       u8 identifier	Identifier byte for application
// @return		NULL not present
//				n address of identifier byte
// *************************************************************************************************
u16* infomem_get_app_addr(u8 identifier)
{
	//start at the first word of payload
	u16* addr= sInfomem.startaddr +2;
	
	//while we have not reached the end
	while(addr<sInfomem.startaddr+2+sInfomem.size)
	{
		//application found, return address
		if( ((u8*)addr)[0] == identifier )
		{
			return addr;
		}
		//step to header of next application
		else
		{
			addr+=((u8*)addr)[1]+1;
		}
	}
	//application not found
	return NULL;
}

// *************************************************************************************************
// @fn          infomem_ready
// @brief       check if infomem is initialized and in sane state, return amount of data present
// @param		none
// @return		-2 no memory structure present
//				-3,-4,-5 data structure error
//				>=0 size of data present
// *************************************************************************************************
s16 infomem_ready()
{
	u8 found_beg=0;
	
	//already checked, trust that and just return size
	if(sInfomem.sane== INFOMEM_SANE)
	{
		return sInfomem.size;
	}
	
	//if address is already set and in right range, look if it is correct, otherwise reset it
	if( (((u16) (sInfomem.startaddr)) >= INFOMEM_START) && (((u16) (sInfomem.startaddr)) < (INFOMEM_START+4*INFOMEM_SEGMENT_SIZE)))
	{
		if(  *(sInfomem.startaddr) == INFOMEM_IDENTIFIER)
		{
			found_beg=1;
		}
		else
		{
			sInfomem.startaddr= NULL;
		}
	}
	
	//search for identifier word of information memory by looping over memory
	if( found_beg == 0 )
	{
		u16* addr;
		for(addr=(u16*)INFOMEM_START; addr<(u16*)(INFOMEM_START+4*INFOMEM_SEGMENT_SIZE); addr++)
		{
			if( *addr == INFOMEM_IDENTIFIER )
			{
				sInfomem.startaddr= addr;
				found_beg=1;
				break;
			}
		}
	}
	
	//give up searching
	if( found_beg ==0)
	{
		return -2;
	}
	
	//read size and maxsize
	sInfomem.size = ((u8*)(sInfomem.startaddr))[2];
	sInfomem.maxsize =  ((u8*)(sInfomem.startaddr))[3];
	
	//check size and maxsize for plausibility
	if(sInfomem.size > sInfomem.maxsize || sInfomem.maxsize > (INFOMEM_START+4*INFOMEM_SEGMENT_SIZE-6 - (u16)sInfomem.startaddr)/2 )
	{
		return -3;
	}
	
	//loop through applications towards the end of the memory
	u16* addr= sInfomem.startaddr+2;
	while(addr<sInfomem.startaddr+2+sInfomem.size)
	{
		addr+=((u8*)addr)[1]+1;
	}
	//return when the sum of the application sizes does not match the total size
	if(addr !=sInfomem.startaddr+sInfomem.size+2)
	{
		return -4;
	}
	
	
	//check if terminator word is in place
	if( sInfomem.startaddr[sInfomem.size+2] != INFOMEM_TERMINATOR)
	{
		return -5;
	}
	
	//exerything seems to be OK
	sInfomem.sane= INFOMEM_SANE;
	sInfomem.not_lock =1;
	return sInfomem.size;
}


// *************************************************************************************************
// @fn          infomem_init
// @brief       write infomem data structure
// @param		u16	start		(word) address of first word of used memory
//				u16	end			(word) address of first word of NOT used memory
// @return		-1 infomem already present
//				-2 addresses not word addresses or out of range
//				-3 memory not empty
//				>0 new maximum size
// *************************************************************************************************
s16 infomem_init(u16 start, u16 end)
{
	if(sInfomem.sane==INFOMEM_SANE)
	{
		return -1;
	}
	
	u16 numwords=(end-start)/2;
	
	//check if address boundaries are usable
	if( start & 0x1 || end &0x1 || end<start || numwords<10 || start < INFOMEM_START || end > INFOMEM_START+4*INFOMEM_SEGMENT_SIZE )
	{
		return -2;
	}
	
	
	int i;
	//check if memory area is empty
	for(i=0; i<numwords; i++)
	{
		if( ((u16*)start)[i] != INFOMEM_ERASED_WORD )
		{
			return -3;
		}
	}
	
	//init struct with standard values
	sInfomem.startaddr = (u16*) start;
	sInfomem.size=0;
	sInfomem.maxsize=(end-start-6)/2;
	sInfomem.not_lock=0;
	
	//prepare the three words of the initial structure
	u16 buf[3]={INFOMEM_IDENTIFIER,((numwords-3) & 0xFF)<<8,INFOMEM_TERMINATOR};
	
	//write the initial structure to memory
	infomem_write_data((u16*)start, buf,3);
	
	//make structure usable
	sInfomem.sane= INFOMEM_SANE;
	sInfomem.not_lock=1;
	
	return sInfomem.maxsize;

};

// *************************************************************************************************
// @fn          infomem_space
// @brief       return amount of free space
// @param		none
// @return		<0 see infomem_ready
//				>=0 available free space (in words)
// *************************************************************************************************
s16 infomem_space()
{
	s16 ret;
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		if((ret=infomem_ready())<0)
		{
			return ret;
		}
	}
	return sInfomem.maxsize -sInfomem.size;
}

// *************************************************************************************************
// @fn          infomem_relocate
// @brief       change start and end address of data storage (can change size)
// @param		u16	start		(word) address of first word of used memory
//				u16	end			(word) address of first word of NOT used memory
// @return		-1 data structure error or memory not initialized
//				-2 temporary error (try again later)
//				-3 address not word addresses
//				-4 addresses out of range
//				-5 new space too small
//				>0 new maximum size
// *************************************************************************************************
s16 infomem_relocate(u16 start, u16 end)
{
	//check if we really have word addresses
	if((start & 0x1) || (end & 0x1))
	{
		return -3;
	}
	
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	//check if range is within memory
	if(end > INFOMEM_START+4*INFOMEM_SEGMENT_SIZE || start < INFOMEM_START)
	{
		return -4;
	}
	//check if new memory range is big enough
	if((u16*)end < (u16*)start +sInfomem.size+3)
	{
		return -5;
	}
	//try to avoid conflicts while writing
	if(sInfomem.not_lock ==0)
	{
		return -2;
	}
	sInfomem.not_lock=0;
	
	
	u16* old_end=sInfomem.startaddr+sInfomem.maxsize+3;
	
	sInfomem.maxsize= (u16*)end - (u16*)start - 3;
	
	//new address of size word
	u16* mod_addr= (u16*)start+1;
	
	//prepare new size word
	u16 buf;
	((u8*)(&buf))[0] = sInfomem.size;
	((u8*)(&buf))[1] = sInfomem.maxsize;
	
	//no relocation, just resize
	if((u16*)start == sInfomem.startaddr)
	{
		infomem_write_data(mod_addr, &buf, 1);
	}
	//left shift (might be with resize)
	else if((u16*)start < sInfomem.startaddr)
	{
		//delete bytes before information memory
		infomem_insert_delete_modify((u16*)start, NULL, (u8)(sInfomem.startaddr-(u16*)start), 0, &mod_addr, &buf, 1, sInfomem.startaddr+3+sInfomem.size, old_end);
	}
	//right shift
	else
	{
		//insert empty bytes before information memory
		infomem_insert_delete_modify(sInfomem.startaddr, NULL, 0, (u8)((u16*)start-sInfomem.startaddr), &mod_addr, &buf, 1, sInfomem.startaddr+3+sInfomem.size, (u16*)start+3+sInfomem.maxsize);
	}
	
	sInfomem.startaddr=(u16*)start;
	
	sInfomem.not_lock=1;
	return sInfomem.maxsize;
}

// *************************************************************************************************
// @fn          infomem_delete_all
// @brief       delete complete (managed) information memory
// @param       none
// @return		-1 data structure error or memory not initialized
//				0 deleted
// *************************************************************************************************
s16 infomem_delete_all(void)
{
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	
	//overwrite complete previously reserved memory with erased bytes
	infomem_insert_delete_modify(sInfomem.startaddr, NULL, sInfomem.maxsize+3, sInfomem.maxsize+3, NULL, NULL, 0, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
	
	sInfomem.sane=0;
	sInfomem.startaddr=NULL;
	sInfomem.size=0;
	sInfomem.maxsize=0;
	return 0;
}

// *************************************************************************************************
// @fn          infomem_app_amount
// @brief       return how much data for the application is available
// @param       u8 identifier	Identifier byte for application
// @return		-1 data structure error or memory not initialized
//				0 application not present
//				n number of words read
// *************************************************************************************************
s16 infomem_app_amount(u8 identifier)
{
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	
	u16* addr= infomem_get_app_addr(identifier);
	if( addr == NULL)
	{
		return 0;
	}
	
	return ((u8*)addr)[1];
}
	
	
// *************************************************************************************************
// @fn          infomem_app_read
// @brief       read count bytes of data with offset for given application into prepared memory
// @param       u8 identifier	Identifier byte for application
//				u16* data		Data array of size>=count words
//				u8 count		number of words to read
//				u8 offset		word offset of data to read
// @return		-1 data structure error or memory not initialized
//				-2 temporary error (try again later)
//				0 offset to big or app not present
//				n number of words read
// *************************************************************************************************
s16 infomem_app_read(u8 identifier, u16* data, u8 count, u8 offset)
{
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	
	//find application
	u16* addr= infomem_get_app_addr(identifier);
	if( addr == NULL)
	{
		return 0;
	}
	
	//read application size
	u8 size=((u8*)addr)[1];
	//check if offset is still within application memory
	if (offset>=size)
	{
		return 0;
	}
	//do not read more data than what is present
	if(count+offset>size)
	{
		count= size-offset;
	}
	//set address to read from
	addr+=offset+1;
	
	int i;
	//copy data
	for(i=0;i<count;i++)
	{
		data[i]=addr[i];
	}
	
	return count;
}

// *************************************************************************************************
// @fn          infomem_app_replace
// @brief       replace all memory content for application by new data
// @param       u8 identifier	Identifier byte for application
//				u16* data		Data array
//				u8 count		number of words
// @return		-1 data structure error or memory not initialized
//				-2 temporary error (try again later)
//				-4 not enough memory
//				n new total size of data in information memory
// *************************************************************************************************
s16 infomem_app_replace(u8 identifier, u16* data, u8 count)
{
	//delete app completely if we have to replace it with zero content.
	if(count ==0)
	{
		return infomem_app_delete(identifier,0);
	}
	
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	if(sInfomem.not_lock ==0)
	{
		return -2;
	}
	sInfomem.not_lock=0;
	
	u8 old_size=0;
	//get address of application
	u16* addr= infomem_get_app_addr(identifier);
	//application is already present, really replace memory content
	if( addr != NULL)
	{
		old_size=((u8*)addr)[1];
		
		//check if new data does fit
		if((s16)sInfomem.size + (s16) count - (s16)old_size > sInfomem.maxsize)
		{
			sInfomem.not_lock=1;
			return -4;
		}
		
		//set global header and application header to be modified
		u16* mod_addr[2]={sInfomem.startaddr+1,addr};
		u16 mod_data[2];
		((u8*)mod_data)[0]=sInfomem.size+count-old_size;
		((u8*)mod_data)[1]=sInfomem.maxsize;
		((u8*)mod_data)[2]=identifier;
		((u8*)mod_data)[3]=count;
		
		//delete old_size words and write count new words instead, also replace headers
		infomem_insert_delete_modify(addr+1, data, old_size, count, mod_addr, mod_data, 2, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
		//store new size
		sInfomem.size=((u8*)mod_data)[0];
	}
	//application not present, add it at the end of the information memory
	else
	{
		//check if new data does fit
		if((s16)sInfomem.size + (s16) count +1 > sInfomem.maxsize)
		{
			sInfomem.not_lock=1;
			return -4;
		}
		
		//prepare header words
		u16* mod_addr[2]={sInfomem.startaddr+1,sInfomem.startaddr+2+sInfomem.size};
		u16 mod_data[2];
		((u8*)mod_data)[0]=sInfomem.size+count+1;
		((u8*)mod_data)[1]=sInfomem.maxsize;
		((u8*)mod_data)[2]=identifier;
		((u8*)mod_data)[3]=count;
		
		//hack: to write the application header and size add it to mod, increase count and decrease data pointer. the first data word at data-1 will not be read
		//this avoids copying data to add a header
		infomem_insert_delete_modify(sInfomem.startaddr+2+sInfomem.size, data-1, 0, count+1, mod_addr, mod_data, 2, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
		//store size
		sInfomem.size=((u8*)mod_data)[0];
	}
	sInfomem.not_lock=1;
	return sInfomem.size;
}

// *************************************************************************************************
// @fn          infomem_app_clear
// @brief       delete all memory content for application
// @param       u8 identifier	Identifier byte for application
// @return		-1 data structure error or memory not initialized
//				-2 temporary error (try again later)
//				0 application not present
//				n new total size of data in information memory
// *************************************************************************************************
s16 infomem_app_clear(u8 identifier)
{
	return infomem_app_delete(identifier,0);
}

// *************************************************************************************************
// @fn          infomem_app_delete
// @brief       delete all memory content beginning with offset
//				delete complete application memory if offset==0
// @param       u8 identifier	Identifier byte for application
//				u8 offset		Word offset of data to delete
// @return		-1 data structure error or memory not initialized
//				-2 temporary error (try again later)
//				-3 offset out of range
//				0 application not present
//				n new total size of data in information memory
// *************************************************************************************************
s16 infomem_app_delete(u8 identifier,u8 offset)
{
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	if(sInfomem.not_lock ==0)
	{
		return -2;
	}
	sInfomem.not_lock=0;
	
	//get address of application
	u16* addr= infomem_get_app_addr(identifier);
	if( addr == NULL)
	{
		sInfomem.not_lock=1;
		return 0;
	}
	//get old size of application
	u8 old_size=((u8*)addr)[1];
	//delete application completely
	if(offset==0)
	{
		//prepare global size header
		u16* mod_addr[1]={sInfomem.startaddr+1};
		u16 mod_data[1];
		((u8*)mod_data)[0]=sInfomem.size-old_size-1;
		((u8*)mod_data)[1]=sInfomem.maxsize;
		
		infomem_insert_delete_modify(addr, NULL, old_size+1, 0, mod_addr, mod_data, 1, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
		//store new size
		sInfomem.size=((u8*)mod_data)[0];
	}
	//let some data be present
	else
	{
		//check if offset is in range
		if(offset>=old_size)
		{
			sInfomem.not_lock=1;
			return -3;
		}
		
		//determine count of data to be deleted
		u8 count_delete=old_size-offset;
		
		//prepare new size headers
		u16* mod_addr[2]={sInfomem.startaddr+1,addr};
		u16 mod_data[2];
		((u8*)mod_data)[0]=sInfomem.size-count_delete;
		((u8*)mod_data)[1]=sInfomem.maxsize;
		((u8*)mod_data)[2]=identifier;
		((u8*)mod_data)[3]=old_size-count_delete;
		
		infomem_insert_delete_modify(addr+1+offset, NULL, count_delete, 0, mod_addr, mod_data, 2, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
		//store new size
		sInfomem.size=((u8*)mod_data)[0];
	}
	sInfomem.not_lock=1;
	return sInfomem.size;
}

// *************************************************************************************************
// @fn          infomem_app_modify
// @brief       modify given bytes of data
//				overwrite count words of data for application beginning from offset
// @param       u8 identifier	Identifier byte for application
//				u16* data		Data array
//				u8 count		Number of words to modyfy
//				u8 offset		Word offset of data to modify
// @return		-1 data structure error or memory not initialized
//				-2 temporary error (try again later)
//				-3 offset too big
//				-4 not enough memory
//				0 application not present (use infomem_app_replace to add new application data)
//				>0 new data syize for application
// *************************************************************************************************
s16 infomem_app_modify(u8 identifier, u16* data, u8 count, u8 offset)
{
	if(sInfomem.sane!=INFOMEM_SANE)
	{
		return -1;
	}
	if(sInfomem.not_lock ==0)
	{
		return -2;
	}
	sInfomem.not_lock=0;
	
	u16* addr= infomem_get_app_addr(identifier);
	
	if( addr == NULL)
	{
		sInfomem.not_lock=1;
		return 0;
	}
	u8 old_size=((u8*)addr)[1];
	
	if(offset>old_size)
	{
		sInfomem.not_lock=1;
		return -3;
	}
	
	//new data does fit in old data, just modify the selected words
	if(count+offset<=old_size)
	{
		infomem_write_data(addr+1+offset,data,count);
		sInfomem.not_lock=1;
		return old_size;
	}
	//new data does not fit, increase size of application's storage
	else
	{
		//calculate number of words that are overwritten
		u8 count_delete=old_size-offset;
		
		//check if new data does fit into memory
		if((s16)sInfomem.size -(s16)count_delete+(s16) count > sInfomem.maxsize)
		{
			sInfomem.not_lock=1;
			return -4;
		}
		
		//prepare new headers with size
		u16* mod_addr[2]={sInfomem.startaddr+1,addr};
		u16 mod_data[2];
		((u8*)mod_data)[0]=sInfomem.size-count_delete+count;
		((u8*)mod_data)[1]=sInfomem.maxsize;
		((u8*)mod_data)[2]=identifier;
		((u8*)mod_data)[3]=old_size-count_delete+count;
		
		infomem_insert_delete_modify(addr+1+offset, data, count_delete, count, mod_addr, mod_data, 2, sInfomem.startaddr+3+sInfomem.size, sInfomem.startaddr+3+sInfomem.maxsize);
		
		//store new size
		sInfomem.size=((u8*)mod_data)[0];
		
		sInfomem.not_lock=1;
		return offset+count;
	}
}

#endif