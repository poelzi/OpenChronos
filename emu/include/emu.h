/*
 * Simple emulator
 */

#include <stdio.h>
#include <stdint.h>

static inline void __no_operation(void) { }
static inline void __disable_interrupt(void) { }
static inline void __enable_interrupt(void) { }
static inline void _BIC_SR_IRQ(int i) { }
static inline void _BIC_SR(int i) { }
static inline void _BIS_SR(int i) { }
static inline void __bic_SR_register_on_exit(int i) { }
#ifdef MYCHRONOS
static inline int __even_in_range(int i, int max) { return i; }
static inline int __get_interrupt_state(void) { return 0; }
static inline void __set_interrupt_state(int state) { }
static inline void __delay_cycles(int num) { }
#else
#if 0
istate_t __get_interrupt_state(void);
void     __set_interrupt_state(istate_t);
unsigned short __even_in_range(unsigned short __value,
                                             unsigned short __bound);
  /* Insert a delay with a specific number of cycles. */
void __delay_cycles(unsigned long __cycles);
#endif
#endif

#define interrupt(foo)

void emu_var_updated(void);

#ifdef CPP_REGS
class reg {
 public:
	volatile unsigned short value;
	reg(void) {	/* constructor */
		printf("Constructing reg at %p\n", this);
	}

	~reg(void) {
	}

	reg &operator|=(const unsigned short &val) {
		value |= val;
		emu_var_updated();
	}
	reg &operator=(const unsigned short &val);
	unsigned short operator&(const unsigned short &val) {
		return value & val;
	}
};
#define CLASS_REG class reg
#else
#define CLASS_REG volatile unsigned short
#endif

extern void emu_redraw(void);
extern char *emu_hint;
extern int emu_inhibit_gui;
extern void emu_timestamp(void);

extern void emu_idle(void);
extern void emu_init(void);

extern void PORT2_ISR(void);
extern void TIMER0_A0_ISR(void);
extern void TIMER0_A1_5_ISR(void);
