// motion.h

#ifndef _MOTION_h
#define _MOTION_h

#include "config.h"
#include "shr16.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "permanent_storage.h"
#include "main.h"
#include <inttypes.h>
#include <stdbool.h>

extern int8_t filament_type[EXTRUDERS];
extern int filament_lookup_table[9][3]; // [X][Y]Two-dimensional Array of extruder and used variables
extern const uint8_t IDLER_PARKING_STEPS;
extern const uint16_t EJECT_PULLEY_STEPS;
extern uint16_t BOWDEN_LENGTH;
extern uint8_t selSGFailCount;
extern uint8_t idlSGFailCount;
extern BowdenLength bowdenLength;


void home(bool doToolSync = false);
void engage_filament_pulley(bool engage);

bool load_filament_withSensor(uint16_t setupBowLen = 0);
bool unload_filament_withSensor(uint8_t extruder = active_extruder);
bool unload_filament_forSetup(uint16_t distance, uint8_t extruder = active_extruder);
void load_filament_into_extruder();

bool set_positions(uint8_t _next_extruder, bool update_extruders = false);
bool steps2setIDL2pos(uint8_t _next_extruder);
bool steps2setSEL2pos(uint8_t _next_extruder);
void set_idler_toLast_positions(uint8_t _next_extruder);
void set_sel_toLast_positions(uint8_t _next_extruder);

bool move_idler(int mm, uint16_t speed = MAX_SPEED_IDL);
bool move_selector(int mm, uint16_t speed = MAX_SPEED_SEL);
void move_pulley(int mm, uint16_t speed = filament_lookup_table[0][0]);
void disableAllSteppers(void);

void eject_filament(uint8_t extruder);
void recover_after_eject();

enum MotReturn {MR_Success, MR_FailedAndRehomed, MR_Failed};
MotReturn homeSelectorSmooth();
MotReturn moveSmooth(uint8_t axis, int mm, int speed, bool rehomeOnFail = true,
                     bool withStallDetection = true, float ACC = ACC_NORMAL,
                     bool withFindaDetection = false, bool withFSensorDetection = false);
MotReturn homeIdlerSmooth(bool toLastFilament = false);
MotReturn homeSelectorSmooth();
#endif
