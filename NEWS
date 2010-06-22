    2010-06-20 - OKAY TO FLASH THE WATCH WITH THIS PROJECT
    It turns out there was never a problem with the build.  
    I discovered later that I had a Windows 7 problem that 
    prevented the BM Firmware Update Tool and also TI's
    BSL_Scripter tool from accessing the the USB FET.

    I've since rebooted my machine and have repeated flashed
    the watch with mspgcc4 project with not problems.

    Paul

    
    ============ FIXED 2010-06-20 ========================
    2010-06-18 - DO NOT FLASH YOUR WATCH WITH THIS BUILD!!
    Since flashing the watch with this build a couple of days ago
    it has been working fine, until I tried to flash it again with
    the USB FET and the BM Firmware Flash Tool , the same tool that 
    worked before I flashed the watch with this build.  
    
    Although I was not able to flash the device with the USB FET
    I was able to flash it with the RFBSL.  But even after reflashing
    it with the factory TI firmware I still could not flash it with 
    the USB FET.

    However, using the IAR Workbench debug process I was able to flash 
    it with the USB FET, and after flashing it this way with the original 
    TI firmware, the USB FET capability is back.  I am able to flash it 
    once again with the BM Firmware Flash Tool via the USB FET.
    
    So a little research is in order to determine how to incorporate the
    USB BSL into this project. 
    
    

    
    2010-06-16 (2:30 AM CDT) - IT WORKS!!!
    I just completed building the project.  I then flashed my watch, 
    and all the functions I have tested work.  Hour and Minutes,
    Seconds, Month and Day, Day of the Week, and YES! even sync.
    
    
    2010-06-15 - Project now compiles and links without errors.  This 
    was accomplished by compiling and linking in separate steps.
    Thanks go to Daniel Poelzleitherner for this contribution.  
    Daniel also contributed Python scripts that convert the build's elf 
    file to a TI txt file.  This function is incorporated into the 
    makefile and is done automatically at the end of the build.
    
    
    ITEMS NEEDING TESTING: (as of 2010-06-14
    
    1) four intrinsic functions had to be written.
    As of 2010-06-13 they are untested and couple of
    them probably don't work yet.
    
    2) bsp_msp430_defs.h - A compiler specific stanza for 
    MSPgcc was added near line #135.  This stanza was copied
    from the previous stanza specific to IAR and modified.  The 
    only modifications were to the __bsp_ENABLE_INTERRUPTS__()
    and __bsp_DISABLE_INTERRUPTS__()  macros.  The other macros 
    may or may not work with MSPgcc.  They do compile without errors or 
    warnings.  The one macro I am not sure about, haven't looked
    into what it means, is __bsp_QUOTED_PRAGMA__(x)
    
    3) FIXED: (see note on 2010-06-15)
    With MSPgcc4 this project compiles cleanly (see minor warning
    in "Before you build the first time:") but there are linking errors 
    "warning: internal error: unsupported relocation error"
    
    
    BEFORE YOU BUILD THE FIRST TIME:
    
    1) Many of the TI source code files #include <intrinsics.h>  MSPgcc does
    not come with an intrinsics.h file.  Therefore the intrinsics.h included 
    in this project will need to be placed in a system directory; e.g., in 
    MSPGCC4\lib\gcc\msp430\4.4.3\include mspgcc4/msp430/include
    
    2) I haven't figured out how to have an empty directory in git so that 
    clones will be complete.  So, before the first build, it is necessary to 
    create the build directory specified in the makefile. 
    
    3) I am using Windows 7, and have experiemented with three different 'make'
    utilities - CygWin, MinGW, and GNU make.  The Unix based CygWin 'make'
    had no problems with a blank in a directory name as long as the blank was
    escaped, but one or both of the DOS versions - MinGW and/or GNU make - did
    have a problem. To avoid this problem I have added an underscore to the
    directory "simpliciti/Applications/application/End Device/".  It is now
    "simpliciti/Applications/application/End_Device/". And the reference in the
    makefile has been adjusted accordingly.  Other than this one thing the code
    should still compile with either IAR, CCS, or MSPgcc4.
    
    4) In general the source files have not been changed in such a way that the
    code will not still compile with either IAR or CCS compilers (except for
    the aforementioned adding of an underscore).  However, I have not included
    the precompiled library files that are need to copile with the code-size 
    limited versions  of IAR and CCS. 

    5) MSPgcc4's signal.h issues an apparently harmless, but annoying warning,
    at line 39.  This warning can fill a page of the output from make, making it
    difficult to identify the real errors.  I commented this line out in my copy 
    of signal.h.  You may want to consider doing the same.

    
    HOW THE PORT WAS DONE:
    
    1) All places where I modified the original code hopefully) are marked with my 
    initials PFS or pfs.  The only actual code changes were to the two area 
    mentioned above in "Items needing testing:" and the #5 below.
    
    2) bluerobin code had to be elimiated because mspgcc cannot link to the provided 
    bluerobin library and the source code is not available.
    
    3) All compiler specific areas that accommodated IAR and CCS were enhanced to 
    accommodate MSPgcc4
    
    4) The Simpliciti *.dat files were converted to *.h files
    
    5) Since I never need to look at my watch to determine the year, but I often
    look to my watch to determine the day of the week (I'm retired now), in date.c 
    I changed the date display to display the day of the week (as a number from 0 to 6).
    The original code is still there, just commented out, just one line.  BTW,
    while doing this I noticed that TI did not use the RTC for the time and date 
    functions which might account for some of the inaccurate timekeeping that has 
    been reported.
    
    
    Paul F. Sehorne
    2010-06-14
    