
# MSP430		(Texas Instruments)
CPU	= MSP430
CC  = msp430-gcc
LD  = msp430-ld
PYTHON := $(shell which python2 || which python)

PROJ_DIR	=.
BUILD_DIR = build
CFLAGS_PRODUCTION = -Os -Wall#-Wl,--gc-sections # -ffunction-sections # -fdata-sections  -fno-inline-functions# -O optimizes
# more optimizion flags
CFLAGS_PRODUCTION +=  -fomit-frame-pointer -fno-force-addr -finline-limit=1 -fno-schedule-insns 
CFLAGS_PRODUCTION += -Wl,-Map=output.map
CFLAGS_DEBUG= -g -Os # -g enables debugging symbol table, -O0 for NO optimization

CC_CMACH	= -mmcu=cc430x6137
CC_DMACH	= -D__MSP430_6137__ -DMRFI_CC430 -D__CC430F6137__ #-DCC__MSPGCC didn't need mspgcc defines __GNUC__
CC_DOPT		= -DELIMINATE_BLUEROBIN
CC_INCLUDE = -I$(PROJ_DIR)/ -I$(PROJ_DIR)/include/ -I$(PROJ_DIR)/gcc/ -I$(PROJ_DIR)/driver/ -I$(PROJ_DIR)/logic/ -I$(PROJ_DIR)/bluerobin/ -I$(PROJ_DIR)/simpliciti/ -I$(PROJ_DIR)/simpliciti/Components/bsp -I$(PROJ_DIR)/simpliciti/Components/bsp/drivers -I$(PROJ_DIR)/simpliciti/Components/bsp/boards/CC430EM -I$(PROJ_DIR)/simpliciti/Components/mrfi -I$(PROJ_DIR)/simpliciti/Components/nwk -I$(PROJ_DIR)/simpliciti/Components/nwk_applications

CC_COPT		=  $(CC_CMACH) $(CC_DMACH) $(CC_DOPT)  $(CC_INCLUDE) 

LOGIC_SOURCE = logic/acceleration.c logic/alarm.c logic/altitude.c logic/battery.c  logic/clock.c logic/date.c logic/menu.c logic/rfbsl.c logic/rfsimpliciti.c logic/stopwatch.c logic/temperature.c logic/test.c logic/user.c logic/phase_clock.c logic/eggtimer.c logic/prout.c logic/vario.c logic/sidereal.c logic/strength.c \
				logic/sequence.c logic/gps.c logic/dst.c

LOGIC_O = $(addsuffix .o,$(basename $(LOGIC_SOURCE)))

DRIVER_SOURCE =  driver/adc12.c driver/buzzer.c driver/display.c driver/display1.c driver/pmm.c driver/ports.c driver/radio.c driver/rf1a.c   driver/timer.c  driver/vti_as.c driver/vti_ps.c driver/dsp.c driver/infomem.c

DRIVER_O = $(addsuffix .o,$(basename $(DRIVER_SOURCE)))

SIMPLICICTI_SOURCE_ODD = simpliciti/Applications/application/End_Device/main_ED_BM.c # changed directory from End Device to End_Device

SIMPLICICTI_SOURCE = $(SIMPLICICTI_SOURCE_ODD) simpliciti/Components/bsp/bsp.c simpliciti/Components/mrfi/mrfi.c simpliciti/Components/nwk/nwk.c simpliciti/Components/nwk/nwk_api.c simpliciti/Components/nwk/nwk_frame.c simpliciti/Components/nwk/nwk_globals.c simpliciti/Components/nwk/nwk_QMgmt.c simpliciti/Components/nwk_applications/nwk_freq.c simpliciti/Components/nwk_applications/nwk_ioctl.c simpliciti/Components/nwk_applications/nwk_join.c simpliciti/Components/nwk_applications/nwk_link.c simpliciti/Components/nwk_applications/nwk_mgmt.c simpliciti/Components/nwk_applications/nwk_ping.c simpliciti/Components/nwk_applications/nwk_security.c 

SIMPLICICTI_O = $(addsuffix .o,$(basename $(SIMPLICICTI_SOURCE)))

MAIN_SOURCE = ezchronos.c  gcc/intrinsics.c 

MAIN_O = ezchronos.o gcc/intrinsics.o 

ALL_O = $(LOGIC_O) $(DRIVER_O) $(SIMPLICICTI_O) $(MAIN_O)

ALL_S = $(addsuffix .s,$(basename $(LOGIC_SOURCE))) $(addsuffix .s,$(basename $(DRIVER_SOURCE))) $(addsuffix .s,$(basename $(SIMPLICICTI_SOURCE)))  \
        $(addsuffix .s,$(basename $(MAIN_SOURCE)))  

EXTRA_O = even_in_range.o 

ALL_C = $(LOGIC_SOURCE) $(DRIVER_SOURCE) $(SIMPLICICTI_SOURCE) $(MAIN_SOURCE)

USE_CFLAGS = $(CFLAGS_PRODUCTION)

CONFIG_FLAGS ?= $(shell cat config.h | grep CONFIG_FREQUENCY | sed 's/.define CONFIG_FREQUENCY //' | sed 's/902/-DISM_US/' | sed 's/433/-DISM_LF/' | sed 's/868/-DISM_EU/')

ifeq (debug,$(findstring debug,$(MAKECMDGOALS)))
USE_CFLAGS = $(CFLAGS_DEBUG)
endif

main: build config.h even_in_range $(ALL_O) $(EXTRA_O) build
	@echo $(findstring debug,$(MAKEFLAGS))
	@echo "Compiling $@ for $(CPU)..."
	$(CC) $(CC_CMACH) $(CFLAGS_PRODUCTION) -o $(BUILD_DIR)/eZChronos.elf $(ALL_O) $(EXTRA_O)
	@echo "Convert to TI Hex file"
	$(PYTHON) tools/memory.py -i build/eZChronos.elf -o build/eZChronos.txt

#debug:	foo
#	@echo USE_CFLAGS = $(CFLAGS_DEBUG)
#	call call_debug

#$(ALL_O): config.h project/project.h $(addsuffix .o,$(basename $@))
#	$(CC) $(CC_COPT) $(USE_CFLAGS) -c $(basename $@).c -o $@

$(ALL_O): %.o: %.c config.h include/project.h
	$(CC) $(CC_COPT) $(USE_CFLAGS) $(CONFIG_FLAGS) -c $< -o $@
#             $(CC) -c $(CFLAGS) $< -o $@


$(ALL_S): %.s: %.o config.h include/project.h
	msp430-objdump -D $< > $@
#             $(CC) -c $(CFLAGS) $< -o $@


debug:	build even_in_range $(ALL_O)
	@echo "Compiling $@ for $(CPU) in debug"
	$(CC) $(CC_CMACH) $(CFLAGS_DEBUG) -o $(BUILD_DIR)/eZChronos.dbg.elf $(ALL_O) $(EXTRA_O)
	@echo "Convert to TI Hex file"
	$(PYTHON) tools/memory.py -i build/eZChronos.dbg.elf -o build/eZChronos.txt

debug_asm: $(ALL_S)
	@echo "Compiling $@ for $(CPU) in debug"

source_index: $(ALL_S)
	for i in $(ALL_S); do echo analyze $$i && m4s init < $$i; done

etags: $(ALL_C) 
	etags $^

even_in_range:
	@echo "Assembling $@ in one step for $(CPU)..."
	$(CC) -D_GNU_ASSEMBLER_ -x assembler-with-cpp -c even_in_range.s -o even_in_range.o

clean: 
	@echo "Removing files..."
	rm -f $(ALL_O)
	rm -rf build/*

build:
	mkdir -p build

config.h:
	$(PYTHON) tools/config.py
	git update-index --assume-unchanged config.h 2> /dev/null || true

config:
	$(PYTHON) tools/config.py
	git update-index --assume-unchanged config.h 2> /dev/null || true

help:
	@echo "Valid targets are"
	@echo "    main"
	@echo "    debug"
	@echo "    clean"
	@echo "    debug_asm"
#rm *.o $(BUILD_DIR)*


#
#----  end of file -------------------------------------------------------------
