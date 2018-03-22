//mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>
#include "config.h"

extern int active_extruder;
extern bool isFilamentLoaded;
extern bool isIdlerParked;


extern bool home_idler();

extern bool home_selector();

extern void demo_switch();
extern bool switch_extruder(int new_extruder);


#endif //_MMCTL_H
