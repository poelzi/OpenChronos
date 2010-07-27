#define EMU_C
#include <cc430x613x.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

void selftest(void);

unsigned char emu_screen[0x20+24]; // = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
char *emu_hint = "Welcome to mychronos";
int emu_inhibit_gui;

static void
clear(void)
{
	printf("\ec");
}

static void
gotoxy(int x, int y)
{
	printf("\e[%d;%dH", y, x);
}

static void
vert(int x, int y, int a)
{
	gotoxy(x, y);   if (a) printf("#"); else printf(".");
	gotoxy(x, y+1); if (a) printf("#"); else printf(".");
}

static void
digit(int x, int y, char bits)
{
#define HORIZ(a) if (bits & a) printf("####"); else printf("....");
	gotoxy(x+1, y+0); HORIZ(0x10);
	gotoxy(x+1, y+3); HORIZ(0x02);
	gotoxy(x+1, y+6); HORIZ(0x80);
	vert(x+0, y+1, bits & 0x01);
	vert(x+5, y+1, bits & 0x20);
	vert(x+0, y+4, bits & 0x04);
	vert(x+5, y+4, bits & 0x40);
}

static void
digit2(int x, int y, char bits)
{
	digit(x, y, (bits >> 4) | (bits << 4));
}

static void
debug_redraw()
{
	int x = 48, y = 2;
	int i;

	gotoxy(x, y);
	printf("%s", emu_hint);
	
	gotoxy(x, y+2);
	printf("Display: ");
	for (i = 0; i < 12; i++ ) {
		printf("%02x", emu_screen[i]);
	}
	gotoxy(x, y+3);
	printf("Buttons %02x ", P2IN);
	gotoxy(x, y+10);
}


/*
PM   ----   ----     ----   ----
    |    | |    |   |    | |    |
^   |    | |    | | |    | |    |
     ----   ----     ----   ----
V   |    | |    | | |    | |    |
    |    | |    |   |    | |    |
     ----   ----     ----   ----

 heart stopw    R     ring   *))))

   ----   ----   ----   ----   ----
| |    | |    | |    | |    | |    |
| |    | |    | |    | |    | |    |
   ----   ----   ----   ----   ----
| |    | |    | |    | |    | |    |
| |    | |    | |    | |    | |    |
   ----   ----   ----   ----   ----
*/

static void
symbol(int x, int y, int draw, const char *s)
{
	gotoxy(x, y);
	if (draw)
		printf("%s", s);
	else {
		int i;
		for (i=0; i<strlen(s); i++)
			printf(".");
	}
}

static int blink_rate = 50;
static int blink;

static int
scrn(int i)
{
	if (blink < blink_rate/2)
		return emu_screen[i];

	//	if (LCDBBLKCTL & LCDBLKMOD0)
		return emu_screen[i] & ~(emu_screen[i+0x20]);

	return emu_screen[i];
}

void
emu_redraw(void)
{
	int x = 2, y = 2;
	int i, symb_x, symb_y;

	digit(x+6,       y+0, scrn(1));
	digit(x+6+7,     y+0, scrn(2));
	symbol(x+6+7+7,  y+2, scrn(0) & 0x20, "#");
	symbol(x+6+7+7,  y+4, scrn(0) & 0x20, "#");
	symbol(x+6+7+7,  y+6, scrn(0) & 0x40, "#");
	digit(x+6+7+9,   y+0, scrn(3));
	digit(x+6+7+9+7, y+0, scrn(5));


	/* FIXME: voltage and DP0 are probably missing.
	   dp0 should be XXX.X in line #1
	   "V" may be linked to battery icon */

	symbol(x+6+7+9+6, y+0, scrn(4) & 0x04, "O");

	symb_x = x+6+7+9+7+8;
	symbol(symb_x,   y+0, scrn(4) & 0x10, "%");
	symbol(symb_x,   y+2, scrn(4) & 0x20, "ft");
	symbol(symb_x,   y+4, scrn(4) & 0x40, "k");
	symbol(symb_x+1, y+4, scrn(6) & 0x02, "m");
	symbol(symb_x+2, y+4, scrn(6) & 0x01, "i");
	symbol(symb_x+1, y+5, scrn(4) & 0x08, "/s");
	symbol(symb_x+4, y+5, scrn(6) & 0x04, "/h");

	gotoxy(x+2, y+0);
	if (scrn(0) & 0x01) {
		if (scrn(0) & 0x02)
			printf("AM");
		else
			printf("PM");
	} else	printf("..");

	symbol(x+2, y+4, scrn(0) & 0x04, "^");
	symbol(x+2, y+6, scrn(0) & 0x08, "v");

	symbol(x+2, y+8, scrn(1) & 0x08, "heart");
	symbol(x+8, y+8, scrn(2) & 0x08, "stopw");
	symbol(x+16,y+8, scrn(0) & 0x80, "Rec");
	symbol(x+24,y+8, scrn(3) & 0x08, "Alarm");
	symbol(x+30,y+8, scrn(4) & 0x08, "*");
	symbol(x+31,y+8, scrn(5) & 0x08, ")");
	symbol(x+32,y+8, scrn(6) & 0x08, ")");

	symb_y = y+18;
	symbol(x+3, symb_y, scrn(10) & 0x80, "TOTAL");
	symbol(x+13, symb_y, scrn(9) & 0x80, "AVG");
	symbol(x+20, symb_y, scrn(7) & 0x80, "MAX");

	/* Missing "V" is linked to "bat" */
	symbol(x+27, symb_y, scrn(6) & 0x80, "bat");

	vert(x, y+11, (scrn(11) & 0x80));
	vert(x, y+14, (scrn(11) & 0x80));
	digit2(x+2,       y+10, scrn(11) & 0x7f);
	symbol(x+2+1*7-1, y+12, scrn(0) & 0x10, "|");
	symbol(x+2+1*7-1, y+14, scrn(0) & 0x10, "|");
	digit2(x+2+1*7,   y+10, scrn(10));
	digit2(x+2+2*7,   y+10, scrn(9));
	symbol(x+2+3*7-1, y+12, scrn(4) & 0x1, "|");
	symbol(x+2+3*7-1, y+14, scrn(4) & 0x1, "|");
	symbol(x+2+3*7-1, y+16, scrn(8) & 0x80, "|");
	digit2(x+2+3*7,   y+10, scrn(8));
	digit2(x+2+4*7,   y+10, scrn(7));

	symb_x=x+2+5*7;
	symbol(symb_x,   y+10, scrn(6) & 0x10, "kcal");
	symbol(symb_x,   y+11, scrn(6) & 0x20, "km");
	symbol(symb_x,   y+12, scrn(6) & 0x40, "mi");

	debug_redraw();
	fflush(stdout);
}

void
process_keys(void)
{
	struct input_event ev;
	while (read(0, &ev, sizeof(ev)) > 0) {
		if (ev.type == EV_KEY) {
			static int key_state;
			int mask, old_state;

			/* q 1, w 4, x 10, z 2, s 8 */
			switch (ev.code) {
			case 0x10: mask = 0x4; break; /* q */
			case 0x2c: mask = 0x2; break; /* z */
			case 0x11: mask = 0x10; break; /* w */
			case 0x1f: mask = 0x8; break; /* s */
			case 0x2d: mask = 0x1; break; /* x */
			default: mask = 0; break;
			}

			old_state = key_state;
			if (ev.value) key_state |= mask; else key_state &= ~mask;
			if (ev.value) P2IFG |= mask; else P2IFG &= ~mask;
			if (ev.value) P2IN |= mask; else P2IN &= ~mask;
			if (ev.value) P2IE |= mask; else P2IE &= ~mask;

			if (key_state != old_state)
				PORT2_ISR();
		}
	}
}

void
emu_idle(void)
{
	pause();
}

static void my_sigio(int ign)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_UNBLOCK, &set, NULL); 

	process_keys();
}

static void itimer(void)
{
	struct itimerval val;

	val.it_interval.tv_sec = 0;
	val.it_value.tv_sec = 0;
	val.it_interval.tv_usec = 10000;
	val.it_value.tv_usec = 10000;

	setitimer(ITIMER_REAL, &val, NULL);
}

static void emu_bad(char *m)
{
	//	printf(m);
	//	exit(1);
}

void emu_var_updated(void)
{
#ifdef CPP_REGS
#define V(x) x.value
#else
#define V(x) x
#endif
	if (LCDBMEMCTL & LCDCLRBM) {
		V(LCDBMEMCTL) &= ~LCDCLRBM;
		memset(emu_screen+0x20, 0, 12);
	}
}


static void my_sigalrm(int ign)
{
	static int last_time, i;

	emu_var_updated();

	if (last_time != time(NULL)) {
		if (!emu_inhibit_gui)
			clear();

		if (TA0CCR0 != 32767)
			emu_bad("Not 1sec timer");
		if (TA0CTL != TASSEL0+MC1+TACLR)
			emu_bad("Not periodic timer");
		if (TA0CCTL0 & CCIE)
			TIMER0_A0_ISR();
		last_time = time(NULL);
	}

	/* should be 32768/100...? */
	for (i=0; i<600; i++) {
		TA0R+=1;

		if ((TA0CCTL4 & CCIE) && (TA0CCR4 == TA0R)) {
			printf("ta0ccr4 = %lx, ta0r = %lx, interrupting\n",
			       TA0CCR4, TA0R);
			TA0IV = 0x08;
			TIMER0_A1_5_ISR();
		}
	}

	if (TA0CCTL2 & CCIE) {
		TA0IV = 0x04;
		TIMER0_A1_5_ISR();
	}

	blink++;
	if (blink == blink_rate)
		blink = 0;

	if (!emu_inhibit_gui)
		emu_redraw();
	itimer();
}

static struct timeval start;

void
emu_timestamp(void)
{
	struct timeval now;
	gettimeofday(&now, NULL);
	printf("%d %03d ", now.tv_sec - start.tv_sec, now.tv_usec/1000);
}


/* /dev/input/event5 */

void
emu_init(void)
{
	gettimeofday(&start, NULL);
	clear();
	signal(SIGIO, my_sigio);
	signal(SIGALRM, my_sigalrm);
	fcntl(0, F_SETOWN, getpid());
	fcntl(0, F_SETFL, O_NONBLOCK | O_ASYNC);
	itimer();
	//selftest();
}

#ifdef CPP_REGS
reg &reg::operator=(const short unsigned int &val)
{
	value = val;
	emu_var_updated();
	return *this;
}
#endif

#define UNDEF(a) void a(void) { printf("Unimplemented function " #a "\n" ); exit(2); }

UNDEF(MRFI_RadioIsr)
UNDEF(BlueRobin_RadioISR_v)
UNDEF(BRRX_TimerTask_v)
UNDEF(BRRX_Init_v)
UNDEF(BRRX_SetPowerdownDelay_v)
UNDEF(BRRX_SetSearchTimeout_v)
UNDEF(BRRX_SetSignalLevelReduction_v)
UNDEF(BRRX_GetState_t)
UNDEF(BRRX_SetID_v)
UNDEF(BRRX_Start_v)
UNDEF(BRRX_GetID_u32)
UNDEF(BRRX_GetHeartRate_u8)
UNDEF(BRRX_GetSpeed_u8)
UNDEF(BRRX_GetDistance_u16)
UNDEF(BRRX_Stop_v)
UNDEF(simpliciti_main_tx_only)
UNDEF(simpliciti_link)
UNDEF(simpliciti_main_sync)
