// mmctl.cpp - multimaterial switcher control
#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "spi.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "Buttons.h"

// public variables:
int active_extruder = -1;  // extruder channel, 0...4
int previous_extruder = -1;
bool isFilamentLoaded = false;
bool isIdlerParked = false;
bool isPrinting = false;
bool isHomed = false;

// private variables:
static int toolChanges = 0;

bool feed_filament()
{
    bool _loaded = false;

    uint8_t current_loading_normal[3] = CURRENT_LOADING_NORMAL;
    uint8_t current_loading_stealth[3] = CURRENT_LOADING_STEALTH;
    uint8_t current_running_normal[3] = CURRENT_RUNNING_NORMAL;
    uint8_t current_running_stealth[3] = CURRENT_RUNNING_STEALTH;
    uint8_t current_holding_normal[3] = CURRENT_HOLDING_NORMAL;
    uint8_t current_holding_stealth[3] = CURRENT_HOLDING_STEALTH;
    

    int _c = 0;
    int _delay = 0;
    engage_filament_pulley(true);
    for (int c = 2; c > 0; c--) {
        if (tmc2130_mode == NORMAL_MODE) {
            tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                             current_loading_normal[AX_PUL]);
        } else {
            tmc2130_init_axis_current_normal(AX_PUL, current_holding_stealth[AX_PUL],
                                             current_loading_stealth[AX_PUL]);
        }
    
            if (moveSmooth(AX_PUL, 1000, 1000, false, false, ACC_FEED_NORMAL, true) == MR_SuccesstoFinda) {  // lower current = disable sg
                delayMicroseconds(1000);
                if (tmc2130_mode == NORMAL_MODE) {
                    tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                                     current_running_normal[AX_PUL]);
                } else {
                    tmc2130_init_axis_current_normal(AX_PUL, current_holding_stealth[AX_PUL],
                                                     current_running_stealth[AX_PUL]);
                }
                moveSmooth(AX_PUL, -600, 1000, false, false, MAX_SPEED_PUL);
                shr16_set_led(1 << 2 * (4 - active_extruder));
                _loaded = true;
                break;
            } else {
                cutOffTip();
            }
    }
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    engage_filament_pulley(false);
    return _loaded;
}

bool switch_extruder_withSensor(int new_extruder)
{

    isPrinting = true;
    bool _return = false;
    if (!isHomed) {
        home();
    }


    if (active_extruder == 5) {
        active_extruder = 4;
        move_selector(-700); // service position
    }



    toolChanges++;

    shr16_set_led(2 << 2 * (4 - active_extruder));

    previous_extruder = active_extruder;
    active_extruder = new_extruder;

    if (previous_extruder == active_extruder) {
        if (!isFilamentLoaded) {
            shr16_set_led(2 << 2 * (4 - active_extruder));
            load_filament_withSensor(); // just load filament if not loaded
            _return = true;
        } else {
            _return = false; // nothing really happened
        }
    } else {
        if (isFilamentLoaded) {
            unload_filament_withSensor(); // unload filament first
        }
        set_positions(previous_extruder,
                      active_extruder); // move idler and selector to new filament position

        shr16_set_led(2 << 2 * (4 - active_extruder));
        load_filament_withSensor(); // load new filament
        _return = true;
    }

    shr16_set_led(0x000);
    shr16_set_led(1 << 2 * (4 - active_extruder));
    return _return;
}

//! @brief select extruder
//!
//! Known limitation is, that if extruder 5 - service position was selected before
//! it is not possible to select any other extruder than extruder 4.
//!
//! @param new_extruder Extruder to be selected
//! @return
bool select_extruder(int new_extruder)
{
    int previous_extruder = active_extruder;
    active_extruder = new_extruder;
    
    bool _return = false;
    if (!isHomed) {
        home();
    }

    shr16_set_led(2 << 2 * (4 - active_extruder));

    if (previous_extruder == active_extruder) {
        if (!isFilamentLoaded) {
            _return = true;
        }
    } else {
        if (new_extruder == EXTRUDERS) {
            move_selector(700); // move to service position
        } else {
            if (previous_extruder == EXTRUDERS) {
                move_selector(-700); // move back from service position
            } else {
                set_positions(previous_extruder,
                              active_extruder); // move idler and selector to new filament position
                //engage_filament_pulley(false); // TODO 3: remove deprecated
            }
        }
        _return = true;
    }


    shr16_set_led(0x000);
    shr16_set_led(1 << 2 * (4 - active_extruder));
    return _return;
}

bool service_position()
{
    // TODO 2: fixme, when abs-coords are implemented
    move_selector(600); // TODO 1: check if 600 is ok!
    return true;
}

void led_blink(int _no)
{
    shr16_set_led(1 << 2 * _no);
    delay(40);
    shr16_set_led(0x000);
    delay(20);
    shr16_set_led(1 << 2 * _no);
    delay(40);

    shr16_set_led(0x000);
    delay(10);
}
