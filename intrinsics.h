#ifndef __INTRINSICS_H
#define __INTRINSICS_H

#include <signal.h>

typedef unsigned short istate_t;

#ifdef __cplusplus
extern "C"
{
#endif

  istate_t __get_interrupt_state(void);
  void     __set_interrupt_state(istate_t);
  unsigned short __even_in_range(unsigned short __value,
                                             unsigned short __bound);

  /* Insert a delay with a specific number of cycles. */
  void __delay_cycles(unsigned long __cycles);

#ifdef __cplusplus
}
#endif


#endif /* __INTRINSICS_H */
