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
    bool _feed = true;
    bool _loaded = false;

    int _c = 0;
    int _delay = 0;
    engage_filament_pulley(true);

    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, 1, 15);
    } else {
        tmc2130_init_axis_current_stealth(AX_PUL, 1, 15);    //probably needs tuning of currents
    }
    if (moveSmooth(AX_PUL, 1500, 1000, false, true, ACC_FEED_NORMAL, true) == MR_SuccesstoFinda) {
        delayMicroseconds(1000);
        moveSmooth(AX_PUL, -600, 1000, false, true, ACC_FEED_NORMAL);
        tmc2130_disable_axis(AX_PUL, tmc2130_mode);
        engage_filament_pulley(false);
        shr16_set_led(1 << 2 * (4 - active_extruder));
        return true;
    }
    return false;
    
    /*do {
        moveSmooth(AX_PUL, 3, MAX_SPEED_PUL, false);

        _c++;
        if (_c > 50) {
            shr16_set_led(2 << 2 * (4 - active_extruder));
        };
        if (_c > 100) {
            shr16_set_led(0x000);
            _c = 0;
            _delay++;
        };


        if (digitalRead(A1) == 1) {
            _loaded = true;
            _feed = false;
        };
        if (buttonClicked() != Btn::none && _delay > 10) {
            _loaded = false;
            _feed = false;
        }
        delayMicroseconds(4000);
    } while (_feed);


    if (_loaded) {
        // unload to PTFE tube
        for (int i = 300; i > 0; i--) { // 570
            moveSmooth(AX_PUL, -3, MAX_SPEED_PUL, false);
            delayMicroseconds(3000);
        }
    }

    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
    engage_filament_pulley(false);
    shr16_set_led(1 << 2 * (4 - active_extruder));
    return true; */
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
        move_selector(-700);
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
