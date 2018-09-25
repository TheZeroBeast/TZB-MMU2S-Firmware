// motion.h

#ifndef _MOTION_h
#define _MOTION_h

#include "config.h"
#include <inttypes.h>

extern void home();
extern bool home_idler();
extern bool home_selector();
 
extern int8_t filament_type[EXTRUDERS];


void park_idler(bool _unpark);

void load_filament_withSensor();
void load_filament_inPrinter();
void unload_filament_withSensor();
void set_positions(int _current_extruder, int _next_extruder);
void init_Pulley();
void do_pulley_step();

void set_idler_dir_down();
void set_idler_dir_up();
void set_pulley_dir_pull();
void set_pulley_dir_push();

void move(int _idler, int _selector, int _pulley);
void move_proportional(int _idler, int _selector);
void eject_filament(int extruder);
void recover_after_eject();



#endif

