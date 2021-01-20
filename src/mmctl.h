// mmctl.h - multimaterial switcher control
#ifndef _MMCTL_H
#define _MMCTL_H

#include <inttypes.h>

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
void engage_filament_pulley(bool engage);
void load_filament_withSensor(uint16_t setupBowLen = 0);
void unload_filament_withSensor(uint8_t extruder = active_extruder);
void unload_filament_forSetup(uint16_t distance, uint8_t extruder = active_extruder);
void load_filament_into_extruder();
void home(bool doToolSync = false);
bool set_positions(uint8_t _next_extruder, bool update_extruders = false);
void set_idler_toLast_positions(uint8_t _next_extruder);
void set_sel_toLast_positions(uint8_t _next_extruder);
bool setIDL2pos(uint8_t _next_extruder);
bool setSEL2pos(uint8_t _next_extruder);
void eject_filament(uint8_t extruder);
void recover_after_eject();
void mmctl_cut_filament(uint8_t x2cut);

#endif //_MMCTL_H
