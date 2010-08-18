// !!!! DO NOT EDIT !!!, use: make config

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define CONFIG_FREQUENCY 902
#define OPTION_TIME_DISPLAY 0
// CONFIG_METRIC_ONLY is not set
#ifndef THIS_DEVICE_ADDRESS
#define THIS_DEVICE_ADDRESS {0xed,0xc0,0xbb,0x25}
#endif // THIS_DEVICE_ADDRESS
// USE_LCD_CHARGE_PUMP is not set
#define USE_WATCHDOG
// DEBUG is not set
#define CONFIG_DAY_OF_WEEK
#define CONFIG_TEST
// CONFIG_EGGTIMER is not set
// CONFIG_PHASE_CLOCK is not set
#define CONFIG_ALTITUDE
// CONFIG_VARIO is not set
// CONFIG_PROUT is not set
#define CONFIG_ACCEL
#define CONFIG_ALARM
#define CONFIG_BATTERY
#define CONFIG_CLOCK
#define CONFIG_DATE
#define CONFIG_RFBSL
#define CONFIG_STOP_WATCH
#define CONFIG_TEMP

#endif // _CONFIG_H_
