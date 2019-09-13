// mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>
#include "config.h"

// public variables:
extern bool isPrinting;
extern bool isHomed;
extern int8_t active_extruder;
extern int8_t activeSelPos;
extern int8_t activeIdlPos;
extern int8_t previous_extruder;
extern bool isIdlerParked;
extern bool homedOnUnload;
extern bool isEjected;
extern uint16_t toolChanges;
extern uint16_t trackToolChanges;

// functions:
void toolChange(int new_extruder);
bool feed_filament(void);
void led_blink(int _no);


#endif //_MMCTL_H
