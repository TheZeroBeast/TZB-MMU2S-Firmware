#ifndef _MAIN_H
#define _MAIN_H


#include <inttypes.h>
#include "config.h"

void manual_extruder_selector();

// system state
extern int8_t sys_state;

// signals from interrupt to main loop
extern uint8_t sys_signals;

// get state of signal (main loop or interrupt)
#define SIG_GET(id) (sys_signals & (1 << id))
// set state of signal (interrupt only)
#define SIG_SET(id) (sys_signals |= (1 << id))
// get state of signal (main loop only)
#define SIG_CLR(id) asm("cli"); sys_signals &= ~(1 << id); asm("sei")


#endif //_MAIN_H
