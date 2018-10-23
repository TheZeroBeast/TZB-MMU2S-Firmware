// mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>
#include "config.h"

// public variables:
extern bool isPrinting;
extern bool isHomed;
extern int active_extruder;
extern int previous_extruder;
extern bool isFilamentLoaded;
extern bool isIdlerParked;

// functions:
bool home_idler();
bool home_selector();
bool switch_extruder_withSensor(int new_extruder);
bool select_extruder(int new_extruder);
bool service_position();
bool feed_filament();
void led_blink(int _no);


#endif //_MMCTL_H
