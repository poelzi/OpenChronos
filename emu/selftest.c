// system
#include "project.h"

#include <string.h>
#include <stdlib.h>

// driver
#include "driver/idle.h"
#include "driver/display.h"
#include "driver/buzzer.h"
#include "driver/ports.h"
#include "driver/timer.h"
#include "driver/pmm.h"
#include "driver/flash.h"

// logic
#include "logic/clock.h"
#include "logic/menu.h"
#include "logic/date.h"
#include "logic/alarm.h"
#include "logic/stopwatch.h"
#include "logic/battery.h"
#include "logic/temperature.h"
#include "logic/altitude.h"
#include "logic/battery.h"
#include "logic/acceleration.h"
#include "logic/doorlock.h"

static void
test_keys(void)
{
	int old;
	printf("Hold W+X\n");
	while (P2IN != 17) {
		while (old == P2IN)
			;
		old = P2IN;
		printf("%02x ", old); fflush(stdout);
	}
}

void
selftest(void)
{
	emu_inhibit_gui = 1;
	emu_timestamp(); printf("selftesting: 0.5 sec sleep\n");
	Timer0_A4_Delay(16000);
	emu_timestamp(); printf("once more 0.5 sec sleep...\n");
	Timer0_A4_Delay(16000);
	emu_timestamp(); printf("ok\n");
	test_keys();
	emu_inhibit_gui = 0;
}
