#ifndef _MAIN_H
#define _MAIN_H


#include <inttypes.h>
#include <stdio.h>
#include "config.h"


// get state of signal (main loop or interrupt)
#define SIG_GET(id) (sys_signals & (1 << id))
// set state of signal (interrupt only)
#define SIG_SET(id) (sys_signals |= (1 << id))
// get state of signal (main loop only)
#define SIG_CLR(id) asm("cli"); sys_signals &= ~(1 << id); asm("sei")


#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


// system state
extern int8_t sys_state;
// signals from interrupt to main loop
extern uint8_t sys_signals;

extern void process_commands(FILE* inout);
extern void process_signals(void);
extern void process_buttons(void);


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)


#endif //_MAIN_H
