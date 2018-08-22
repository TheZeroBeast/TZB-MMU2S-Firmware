//mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>
#include "config.h"

extern int lengthCorrection;
extern bool isPrinting;
extern bool isHomed;
extern int toolChanges;

extern int active_extruder;
extern int previous_extruder;
extern bool isFilamentLoaded;
extern bool isIdlerParked;


extern bool home_idler();
extern bool home_selector();

extern bool switch_extruder_withSensor(int new_extruder);

extern bool select_extruder(int new_extruder);
extern bool service_position(bool service);

extern bool feed_filament();

void led_blink(int _no);


#endif //_MMCTL_H
