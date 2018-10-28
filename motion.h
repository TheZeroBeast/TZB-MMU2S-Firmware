// motion.h

#ifndef _MOTION_h
#define _MOTION_h

#include "config.h"
#include <inttypes.h>
#include <stdbool.h>


extern int8_t filament_type[EXTRUDERS];

void home();
void engage_filament_pulley(bool engage);

bool isFilamentInFinda();
bool load_filament_withSensor();
void load_filament_into_extruder();

bool unload_filament_withSensor();
void set_positions(int _current_extruder, int _next_extruder);
bool reset_positions(uint8_t axis, int _current_extruder, int _next_extruder,  float acc = ACC_NORMAL);
bool cutOffTip();
void init_Pulley();

void move_idler(int steps, uint16_t speed = MAX_SPEED_IDL);
void move_selector(int steps, uint16_t speed = MAX_SPEED_SEL);
void move_pulley(int steps, uint16_t speed = MAX_SPEED_PUL);

void eject_filament(int extruder);
void recover_after_eject();

enum MotReturn {MR_Success, MR_FailedAndRehomed, MR_Failed, MR_SuccesstoFinda};
MotReturn homeSelectorSmooth();
MotReturn moveSmooth(uint8_t axis, int steps, int speed,
                     bool rehomeOnFail = true, bool withStallDetection = true, float ACC = ACC_NORMAL, bool withFindaDetection = false, bool withMK3FSensorDetection = false);
MotReturn homeIdlerSmooth();
MotReturn homeSelectorSmooth();
#endif
