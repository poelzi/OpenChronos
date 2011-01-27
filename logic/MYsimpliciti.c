// MYsimpliciti


#if defined (CONFIG_USEPPT) || defined (CONFIG_EGGTIMER) || (CONFIG_ACCEL)
#define SIMPLICITI_TX_ONLY_REQ
#endif


/*
smplStatus_t SMPL_Link(0);
}

smplStatus_t SMPL_Init(uint8_t (*callback)(linkID_t));

smplStatus_t SMPL_SendOpt(lid, *msg, len,opt);

smplStatus_t SMPL_Receive(lid, *msg, *len);

smplStatus_t SMPL_Ioctl(object, action, *val);
*/

// Include section

// system
#include "project.h"


// forward declarations/prototype section
// used in main.c and rf1a.c
void __delay_cycles(u16 cycles);

// used in adc12.c
u8 __even_in_range(u16 vector, u8 unknown);

// used in radio.c
void MRFI_RadioIsr(void); 

//used in rf1a.c
u16 __get_interrupt_state(void);
void __disable_interrupt(void);
void __set_interrupt_state(u16 int_state);

// used in rfsimpliciti.c
void simpliciti_link(void);
void simpliciti_main_sync(void);
void simpliciti_main_tx_only(void);


// functions
void __delay_cycles(u16 cycles) {}
u8 __even_in_range(u16 vector, u8 unknown) {}
void MRFI_RadioIsr(void){}
u16 __get_interrupt_state(void) {}
void __disable_interrupt(void) {}
void __set_interrupt_state(u16 int_state) {}
void simpliciti_link() {}
void simpliciti_main_sync() {}
void simpliciti_main_tx_only() {}

// forward declarations/prototype section
/*
void reset_rf(void);
void sx_rf(u8 line);
void sx_ppt(u8 line);
void sx_sync(u8 line);
void display_rf(u8 line, u8 update);
void display_ppt(u8 line, u8 update);
void display_sync(u8 line, u8 update);
void send_smpl_data(u16 data);
u8 is_rf(void);
*/
/*
void reset_rf() {}
void sx_rf(u8 line) {}
void sx_ppt(u8 line) {}
void sx_sync(u8 line){}
void display_rf(u8 line, u8 update) {}
void display_ppt(u8 line, u8 update) {}
void display_sync(u8 line, u8 update) {}
void send_smpl_data(u16 data){}
u8 is_rf(void) {}
*/
