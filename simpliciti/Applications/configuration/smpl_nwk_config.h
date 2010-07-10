#ifndef SMPL_NWK_CONFIG
#define SMPL_NWK_CONFIG
#include "smpl_config.h"  //pfs added

#define MAX_HOPS  3 
#define MAX_HOPS_FROM_AP  1 
#define MAX_NWK_PAYLOAD  9 
#define MAX_APP_PAYLOAD  19 
#define DEFAULT_LINK_TOKEN  0x01020304 
#define DEFAULT_JOIN_TOKEN  0x05060708 
#define APP_AUTO_ACK
// if a app needs EXTENDED_API it should enable it here 
#define xEXTENDED_API 
#define xSMPL_SECURE 
#define xNVOBJECT_SUPPORT 
#define SW_TIMER

#endif
