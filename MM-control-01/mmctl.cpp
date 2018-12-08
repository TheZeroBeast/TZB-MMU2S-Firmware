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
int active_extruder = -1;
int previous_extruder = -1;
bool isFilamentLoaded = false;
bool isIdlerParked = false;
bool isPrinting = false;
bool isHomed = false;
bool homedOnUnload = false;

// private variables:
static int toolChanges = 0;
int trackToolChanges = 0;

bool feed_filament(void)
{
    bool _loaded = false;

    int _c = 0;
    engage_filament_pulley(true);

    while (!_loaded) {

        if (moveSmooth(AX_PUL, 4000, filament_lookup_table[5][filament_type[active_extruder]], false, true, ACC_NORMAL, true) == MR_Success) {
            delay(5);
            moveSmooth(AX_PUL, filament_lookup_table[3][filament_type[active_extruder]], filament_lookup_table[5][filament_type[active_extruder]], false, false, ACC_NORMAL);
            shr16_clr_led();
            shr16_set_led(1 << 2 * (4 - active_extruder));
            _loaded = true;
            break;
        } else {
            if (_c < 1) {                     // Two attempt to load then give up
                fixTheProblem(false);
                engage_filament_pulley(true);
            } else {
                _loaded = false;
                break;
            }
            _c++;
        }
    }
    shr16_clr_ena(AX_PUL);
    engage_filament_pulley(false);
    return _loaded;
}

bool toolChange(int new_extruder)
{
    bool _return = false;
    isPrinting = true;

    shr16_clr_led();
    shr16_set_led(2 << 2 * (4 - active_extruder));

    previous_extruder = active_extruder;
    active_extruder = new_extruder;
    
    if (previous_extruder == active_extruder) {
        if (!isFilamentLoaded) {
            shr16_clr_led();
            shr16_set_led(2 << 2 * (4 - active_extruder));
            load_filament_at_toolChange = true;
            _return = true;
        } else {
            _return = true; // nothing really happened
        }
    } else {
        if (isFilamentLoaded) unload_filament_withSensor(previous_extruder); //unload filament if you need to
        if (trackToolChanges == TOOLSYNC) { // Home every period TOOLSYNC
            home(true);
            // move idler and selector to new filament position
        } else if (!homedOnUnload) set_positions(previous_extruder, active_extruder, true);
        toolChanges++;
        trackToolChanges ++;
        shr16_clr_led();
        shr16_set_led(2 << 2 * (4 - active_extruder));
        load_filament_at_toolChange = true;
        homedOnUnload = false;
        _return = true;
    }

    shr16_clr_led();
    shr16_set_led(1 << 2 * (4 - active_extruder));
    return _return;
}

void led_blink(int _no)
{
    shr16_clr_led();
    shr16_set_led(1 << 2 * _no);
    delay(40);
    shr16_clr_led();
    delay(20);
    shr16_set_led(1 << 2 * _no);
    delay(40);
    shr16_clr_led();
    delay(10);
}
