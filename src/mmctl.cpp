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
int8_t active_extruder = -1;
int8_t activeSelPos = -1;
int8_t activeIdlPos = -1;
int8_t previous_extruder = -1;
uint16_t toolChanges = 0;
uint16_t trackToolChanges = 0;
bool isIdlerParked = false;
bool isPrinting = false;
bool isEjected = false;
bool isHomed = false;
bool homedOnUnload = false;

bool feed_filament(void)
{
    bool _loaded = false;
    if (!isHomed && !isFilamentLoaded()) home(true);
    if (!isFilamentLoaded()) {
        int _c = 0;
        shr16_clr_led();
        shr16_set_led(2 << 2 * (4 - active_extruder));
        engage_filament_pulley(true);
        while (!_loaded) {
    
            if (moveSmooth(AX_PUL, 4000, filament_lookup_table[5][filament_type[active_extruder]], false, true, GLOBAL_ACC, true) == MR_Success) {
                delay(10);
                moveSmooth(AX_PUL, filament_lookup_table[3][filament_type[active_extruder]], filament_lookup_table[5][filament_type[active_extruder]], false, false, GLOBAL_ACC);
                shr16_clr_led();
                shr16_set_led(1 << 2 * (4 - active_extruder));
                _loaded = true;
                break;
            } else {
                if (_c < 1) {                     // Two attempt to load then give up
                    fixTheProblem();
                    engage_filament_pulley(true);
                } else {
                    engage_filament_pulley(false);
                    _loaded = false;
                    break;
                }
                _c++;
            }
        }
        shr16_clr_ena(AX_PUL);
        engage_filament_pulley(false);
    } else {
        txPayload((unsigned char*)"Z1---");
        txACKMessageCheck();
        delay(1500);
        txPayload((unsigned char*)"ZZZ--");
        txACKMessageCheck();
    }
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
        if (!isFilamentLoaded()) {
            if (!isHomed) home(true);
            shr16_clr_led();
            shr16_set_led(2 << 2 * (4 - active_extruder));
            load_filament_withSensor();
            _return = true;
        } else {
            _return = true; // nothing really happened
        }
    } else {
        if (isFilamentLoaded()) unload_filament_withSensor(previous_extruder); //unload filament if you need to
        if ((trackToolChanges == TOOLSYNC) || !isHomed) { // Home every period TOOLSYNC
            home(true);
        // move idler and selector to new filament position
        } else if (!homedOnUnload) {
          if (!set_positions(active_extruder, true)) {
            home(true);
          }
        }
        toolChanges++;
        trackToolChanges++;
        uint8_t toolChangesUpper = (0xFF & (toolChanges >> 8));
        uint8_t toolChangesLower = (0xFF & toolChanges);
        unsigned char txTCU[5] = {'T',toolChangesUpper, toolChangesLower, BLK, BLK};
        txPayload(txTCU);
        txACKMessageCheck();
        shr16_clr_led();
        shr16_set_led(2 << 2 * (4 - active_extruder));
        load_filament_withSensor();
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
