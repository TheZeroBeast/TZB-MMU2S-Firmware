//! @file

#include "Buttons.h"
#include "shr16.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "permanent_storage.h"
#include "main.h"

const int ButtonPin = A2; // we use an analog input with different DC-levels for each button

//! @brief Select filament for bowden length calibration
//!
//! Filaments are selected by left and right buttons, calibration is activated by middle button.
//! Park position (one behind last filament) can be also selected.
//! Activating calibration in park position exits selector.
//!
void settings_select_filament()
{
    while (1) {
        manual_extruder_selector();

        if (Btn::middle == buttonClicked()) {
            shr16_set_led(2 << 2 * (4 - active_extruder));
            delay(500);
            if (Btn::middle == buttonClicked()) {
                select_extruder(4);
                select_extruder(0);
                return;
            }
        }
    }
}

//! @brief Show setup menu
//!
//! Items are selected by left and right buttons, activated by middle button.
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 11 | 00 | 00 | 00 | 01 | initial state, no action
//! 11 | 00 | 00 | 01 | 00 | setup bowden length
//! 11 | 00 | 01 | 00 | 00 | erase EEPROM if unlocked
//! 11 | 01 | 00 | 00 | 00 | unlock EEPROM erase
//! 11 | 00 | 00 | 00 | 00 | exit setup menu
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//!
void setupMenu()
{
    shr16_set_led(0x000);
    delay(200);
    shr16_set_led(0x2aa);
    delay(1200);
    shr16_set_led(0x000);
    delay(600);

    int _menu = 0;
    bool _exit = false;
    bool eraseLocked = true;



    do {
        shr16_set_led(1 << 2 * 4);
        delay(1);
        shr16_set_led(2 << 2 * 4);
        delay(1);
        shr16_set_led(2 << 2 * _menu);
        delay(1);

        switch (buttonClicked()) {
        case Btn::right:
            if (_menu > 0) {
                _menu--;
                delay(800);
            }
            break;
        case Btn::middle:

            switch (_menu) {
            case 1:
                settings_select_filament();
                _exit = true;
                break;
            case 2:
                if (!eraseLocked) {
                    //BowdenLength::eraseAll();
                    _exit = true;
                }
                break;
            case 3: //unlock erase
                eraseLocked = false;
                break;
            case 4: // exit menu
                _exit = true;
                break;
            }
            break;
        case Btn::left:
            if (_menu < 4) {
                _menu++;
                delay(800);
            }
            break;
        default:
            break;
        }

    } while (!_exit);


    shr16_set_led(0x000);
    delay(400);
    shr16_set_led(0x2aa);
    delay(400);
    shr16_set_led(0x000);
    delay(400);

    shr16_set_led(0x000);
    shr16_set_led(1 << 2 * (4 - active_extruder));
}

//! @brief Is button pushed?
//!
//! @return button pushed

Btn buttonClicked()
{
    // TODO 1: is something like a minimum hold time needed? could be that a
    // rising edge falls temporarily into a wrong class
    int raw = analogRead(ButtonPin);

    if (raw < 50) {
        return Btn::right;
    }
    if (raw > 80 && raw < 100) {
        return Btn::middle;
    }
    if (raw > 160 && raw < 180) {
        return Btn::left;
    }

    return Btn::none;
}

