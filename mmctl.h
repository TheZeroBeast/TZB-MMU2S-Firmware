//mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>
#include "config.h"


extern int active_extruder;

extern bool home_idler();

extern bool home_selector();

extern bool move_selector();

extern bool home_();

extern bool switch_extruder(int new_extruder);


#endif //_MMCTL_H
