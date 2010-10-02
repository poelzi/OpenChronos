#ifndef STRENGTH_H_
#define STRENGTH_H_

#include "project.h"

#define STRENGTH_COUNTDOWN_SECS 5
#define STRENGTH_THRESHOLD_1 (60+STRENGTH_COUNTDOWN_SECS)
#define STRENGTH_THRESHOLD_2 (90+STRENGTH_COUNTDOWN_SECS)
#define STRENGTH_THRESHOLD_END (200+STRENGTH_COUNTDOWN_SECS)

#define STRENGTH_BUZZER_ON_TICKS (CONV_MS_TO_TICKS(30))
#define STRENGTH_BUZZER_OFF_TICKS (CONV_MS_TO_TICKS(100))

typedef struct
{
	struct {
		unsigned running : 1;
		unsigned redisplay_requested : 1;
	} flags;
	u8 num_beeps;
	u8 seconds_since_start;
	u8 time[4];
} strength_data_t;

extern strength_data_t strength_data;

static inline u8 is_strength() { return strength_data.flags.running ; }
void strength_tick();
void display_strength_time(u8 line, u8 update);

void strength_sx(u8);
u8 strength_display_needs_updating(void);

#endif
