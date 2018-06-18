// motion.h

#ifndef _MOTION_h
#define _MOTION_h

extern bool home();
extern bool home_idler();
extern bool home_selector();
 




void park_idler(bool _unpark);

void load_filament_withSensor();
void load_filament_inPrinter();
void unload_filament_withSensor();
void set_positions(int _current_extruder, int _next_extruder);
void init_Pulley();

#endif

