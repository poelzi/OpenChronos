
    This port consists of the following:
    0) All places where I modified the original code
    (hopefully) are marked with my initials PFS or pfs.
    The only actual code changes were to write four
    functions to replace the intrinsic functions used
    by TI and IAR.
    1) bluerobin code had to be elimiated because
    mspgcc cannot link to the provided bluerobin library
    and the source code is not available.
    2) four intrinsic functions had to be written.
    As of 2010-06-13 they are untested and couple of
    them probably don't work yet.
    3) All compiler specific areas what accommodated
    IAR and CCS were enhanced to accommodate MSPgcc4
    4) The Simpliciti *.dat files were converted to *.h files
	5) Many of the TI source code hmave 
	#include <intrinsics.h>  Therefore the intrinsics.h 
	included in this project will need to be placed in a 
	system directory; e.g., in 
	MSPGCC4\lib\gcc\msp430\4.4.3\include mspgcc4/msp430/include

