
# MSP430		(Texas Instruments)
CPU	= MSP430
CC  = msp430-gcc
LD  = msp430-ld

MKDIR = mkdir

PROJ_DIR	=.
DEBUG_DIR	=debug/
RELEASE_DIR =release/
CFLAGS_PRODUCTION = -B -s -Os # -O optimizes
CFLAGS_DEBUG= -g -O0 # -g enables debugging symbol table, -O0 for NO optimization
CC_CMACH	= -mmcu=cc430x6137
CC_DMACH	= -D__MSP430_6137__  -DISM_US -DMRFI_CC430 -D__CC430F6137__ #-DCC__MSPGCC didn't need mspgcc defines __GNUC__
CC_DOPT		= -DELIMINATE_BLUEROBIN
CC_INCLUDE = -I$(PROJ_DIR)/ -I$(PROJ_DIR)/include/ -I$(PROJ_DIR)/driver/ -I$(PROJ_DIR)/logic/ -I$(PROJ_DIR)/bluerobin/ -I$(PROJ_DIR)/simpliciti/ -I$(PROJ_DIR)/simpliciti/Components/bsp -I$(PROJ_DIR)/simpliciti/Components/bsp/drivers -I$(PROJ_DIR)/simpliciti/Components/bsp/boards/CC430EM -I$(PROJ_DIR)/simpliciti/Components/mrfi -I$(PROJ_DIR)/simpliciti/Components/nwk -I$(PROJ_DIR)/simpliciti/Components/nwk_applications

CC_COPT		=  $(CC_CMACH) $(CC_DMACH) $(CC_DOPT)  $(CC_INCLUDE) 

LOGIC_SOURCE = logic/acceleration.c logic/alarm.c logic/altitude.c logic/battery.c  logic/clock.c logic/date.c logic/menu.c logic/rfbsl.c logic/rfsimpliciti.c logic/stopwatch.c logic/temperature.c logic/test.c logic/user.c 

LOGIC_O = $(addsuffix .o,$(basename $(LOGIC_SOURCE)))

DRIVER_SOURCE =  driver/adc12.c driver/buzzer.c driver/display.c driver/display1.c driver/pmm.c driver/ports.c driver/radio.c driver/rf1a.c   driver/timer.c  driver/vti_as.c driver/vti_ps.c 

DRIVER_O = $(addsuffix .o,$(basename $(DRIVER_SOURCE)))

SIMPLICICTI_SOURCE_ODD = simpliciti/Applications/application/End_Device/main_ED_BM.c # changed directory from End Device to End_Device

SIMPLICICTI_SOURCE = $(SIMPLICICTI_SOURCE_ODD) simpliciti/Components/bsp/bsp.c simpliciti/Components/mrfi/mrfi.c simpliciti/Components/nwk/nwk.c simpliciti/Components/nwk/nwk_api.c simpliciti/Components/nwk/nwk_frame.c simpliciti/Components/nwk/nwk_globals.c simpliciti/Components/nwk/nwk_QMgmt.c simpliciti/Components/nwk_applications/nwk_freq.c simpliciti/Components/nwk_applications/nwk_ioctl.c simpliciti/Components/nwk_applications/nwk_join.c simpliciti/Components/nwk_applications/nwk_link.c simpliciti/Components/nwk_applications/nwk_mgmt.c simpliciti/Components/nwk_applications/nwk_ping.c simpliciti/Components/nwk_applications/nwk_security.c 

SIMPLICICTI_O = $(addsuffix .o,$(basename $(SIMPLICICTI_SOURCE)))

MAIN_SOURCE = even_in_range.o ezchronos.c intrinsics.c 

MAIN_O = even_in_range.o ezchronos.o intrinsics.o 

ALL_O = $(LOGIC_O) $(DRIVER_O) $(SIMPLICICTI_O) $(MAIN_O)

$(ALL_O):
	$(CC) $(CC_COPT) $(CFLAGS_PRODUCTION) -c $(basename $@).c -o $@

main:	even_in_range $(ALL_O)
	@echo "Compiling $@ in one step for $(CPU)..."
	$(MKDIR) -p $(RELEASE_DIR)
	#$(CC) $(CFLAGS_PRODUCTION) $(CC_COPT) $(CC_LINK) $(MAIN_SOURCE)  $(SIMPLICICTI_SOURCE) $(LOGIC_SOURCE) $(DRIVER_SOURCE)	-o $(RELEASE_DIR)eZChronos.elf 
	msp430-gcc $(CC_CMACH)  -s -Os -o $(RELEASE_DIR)eZChronos.elf $(ALL_O) 
	
debug:	even_in_range
	@echo "Assembling $@ in one step for $(CPU)..."
	@echo $(LOGIC_O)
	$(MKDIR) -p $(DEBUG_DIR)
	$(CC) $(CFLAGS_DEBUG) $(CC_COPT) $(CC_LINK) $(MAIN_SOURCE)  $(SIMPLICICTI_SOURCE) $(LOGIC_SOURCE) $(DRIVER_SOURCE)	-o $(DEBUG_DIR)eZChronos.elf 

even_in_range:
	@echo "Assembling $@ in one step for $(CPU)..."
	msp430-gcc -D_GNU_ASSEMBLER_ -x assembler-with-cpp -c even_in_range.s -o even_in_range.o

	
clean: 
	@echo "Removing files..."
	rm -f $(ALL_O)
#rm *.o $(BUILD_DIR)*

	
	
#
#----  end of file -------------------------------------------------------------
