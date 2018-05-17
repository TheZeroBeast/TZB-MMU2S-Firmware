//mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>
#include "config.h"

extern bool isPrinting;
extern bool isHomed;

extern int active_extruder;
extern bool isFilamentLoaded;
extern bool isIdlerParked;


extern bool home_idler();
extern bool home_selector();

extern bool switch_extruder(int new_extruder);
extern bool select_extruder(int new_extruder);



#endif //_MMCTL_H
