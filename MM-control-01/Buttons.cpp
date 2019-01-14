//! @file

#include "Buttons.h"

const int ButtonPin = A2; // we use an analog input with different DC-levels for each button

void settings_bowden_length();
void settings_fsensor_length();

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
    shr16_clr_led();
    delay(200);
    shr16_set_led(0x2aa);
    delay(1200);
    shr16_clr_led();

    uint8_t _menu = 0;
    uint8_t _menu_last_cycle = 10;
    bool _exit = false;
    bool eraseLocked = true;
    inErrorState = true;

    do {
        process_commands();
        shr16_clr_led();
        shr16_set_led((1 << 2 * 4) | (2 << 2 * 4) | (2 << 2 * _menu));
        if (_menu != _menu_last_cycle) {
               if (_menu == 0) txPayload((unsigned char*)"X1-");
          else if (_menu == 1) txPayload((unsigned char*)"X2-");
          else if (_menu == 2) txPayload((unsigned char*)"X5-");
          else if (_menu == 3) txPayload((unsigned char*)"X6-");
          else if (_menu == 4) txPayload((unsigned char*)"X7-");
        }
        _menu_last_cycle = _menu;

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
                if (!isFilamentLoaded()) {
                    settings_bowden_length();
                    _exit = true;
                }
                break;
            case 2:
                if (!eraseLocked)
                {
                    eepromEraseAll();
                    _exit = true;
                }
                break;
            case 3: // unlock erase
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

    shr16_clr_led();
    delay(400);
    shr16_set_led(0x2aa);
    delay(400);
    shr16_clr_led();
    process_commands();
    txPayload((unsigned char*)"ZZZ");
    shr16_set_led(1 << 2 * (4 - active_extruder));
}

//! @brief Set bowden length
//!
//! button         | action
//! -------------- | ------
//! Left   LOADED  | increase bowden length / feed more filament
//! Left UNLOADED  | store bowden length to EEPROM and exit
//! Right          | decrease bowden length / feed less filament
//! Middle         | Load/Unload to recheck length
//!
//! This state is indicated by following LED pattern:
//!
//! RG | RG | RG | RG | RG
//! -- | -- | -- | -- | --
//! bb | 00 | 00 | 0b | 00
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
//!
void settings_bowden_length()
{
    // load filament to end of detached bowden tube to check correct length
    if (!isHomed) home();
    else set_positions(active_extruder, 0, true);
    for (uint8_t i = 0; i < 5; i++) bowdenLength.increase();
    uint8_t tempBowLenUpper = (0xFF & ((bowdenLength.m_length - 150u) >> 8));
    uint8_t tempBowLenLower = (0xFF & (bowdenLength.m_length - 150u));
    unsigned char tempW[3] = {'W', tempBowLenUpper, tempBowLenLower};
    unsigned char tempV[3] = {0,0,0};
    txPayload(tempW);    
    goto loop2;
loop:
    tempBowLenUpper = (0xFF & ((bowdenLength.m_length - 150u) >> 8));
    tempBowLenLower = (0xFF & (bowdenLength.m_length - 150u));
    tempV[0] = 'V';
    tempV[1] = tempBowLenUpper;
    tempV[2] = tempBowLenLower;
    txPayload(tempV);
    load_filament_withSensor(bowdenLength.m_length);

    tmc2130_init_axis_current_normal(AX_PUL, 1, 30, false);
    do
    {
        switch (buttonClicked())
        {
        case Btn::right:
            if (bowdenLength.decrease())
            {
                move_pulley(-bowdenLength.stepSize);
                tempBowLenUpper = (0xFF & ((bowdenLength.m_length - 150u) >> 8));
                tempBowLenLower = (0xFF & (bowdenLength.m_length - 150u));
                tempV[0] = 'V';
                tempV[1] = tempBowLenUpper;
                tempV[2] = tempBowLenLower;
                txPayload(tempV);
                delay(200);
            }
            break;

        case Btn::left:
            if (bowdenLength.increase())
            {
                move_pulley(bowdenLength.stepSize);
                tempBowLenUpper = (0xFF & ((bowdenLength.m_length - 150u) >> 8));
                tempBowLenLower = (0xFF & (bowdenLength.m_length - 150u));
                tempV[0] = 'V';
                tempV[1] = tempBowLenUpper;
                tempV[2] = tempBowLenLower;
                txPayload(tempV);
                delay(200);
            }
            break;
        default:
            break;
        }

        shr16_set_led((1 << 2 * 4) | (2 << 2 * 4) | (2 << 2 * 1));
    } while (buttonClicked() != Btn::middle);
    tempBowLenUpper = (0xFF & ((bowdenLength.m_length - 150u) >> 8));
    tempBowLenLower = (0xFF & (bowdenLength.m_length - 150u));
    tempW[0] = 'W';
    tempW[1] = tempBowLenUpper;
    tempW[2] = tempBowLenLower;
    txPayload(tempW);
    unload_filament_withSensor();
loop2:
    switch (buttonClicked()) {
    case Btn::middle:
        goto loop;
        break;
    case Btn::left:
        break;
    case Btn::right:
        settings_fsensor_length();
        break;
    default:
        goto loop2;
    }
    for (uint8_t i = 0; i < 5; i++) bowdenLength.decrease();
    bowdenLength.~BowdenLength();
    BOWDEN_LENGTH = BowdenLength::get();
}

void settings_fsensor_length()
{
    // load filament to FSensor then to middle of Bondtech to set middle

    uint8_t tempFSensLenUpper = (0xFF & (bowdenLength.m_FSensorSteps >> 8));
    uint8_t tempFSensLenLower = (0xFF & bowdenLength.m_FSensorSteps);
    unsigned char tempB[3] = {'B', tempFSensLenUpper, tempFSensLenLower};
    txPayload(tempB);
    load_filament_withSensor();    
    tmc2130_init_axis_current_normal(AX_PUL, 1, 30, false);
    do
    {
        switch (buttonClicked())
        {
        case Btn::right:
            if (bowdenLength.decreaseFSensor())
            {
                move_pulley(-bowdenLength.stepSizeFSensor);
                tempFSensLenUpper = (0xFF & (bowdenLength.m_FSensorSteps >> 8));
                tempFSensLenLower = (0xFF & bowdenLength.m_FSensorSteps);
                unsigned char tempB[3] = {'B', tempFSensLenUpper, tempFSensLenLower};
                txPayload(tempB);
                delay(200);
            }
            break;

        case Btn::left:
            if (bowdenLength.increaseFSensor())
            {
                move_pulley(bowdenLength.stepSizeFSensor);
                tempFSensLenUpper = (0xFF & (bowdenLength.m_FSensorSteps >> 8));
                tempFSensLenLower = (0xFF & bowdenLength.m_FSensorSteps);
                unsigned char tempB[3] = {'B', tempFSensLenUpper, tempFSensLenLower};
                txPayload(tempB);
                delay(200);
            }
            break;
        default:
            break;
        }

        shr16_set_led((1 << 2 * 4) | (2 << 2 * 4) | (2 << 2 * 1));
    } while (buttonClicked() != Btn::middle);
    
    //uint16_t tempFSensorSteps = (0xFF & bowdenLength.m_FSensorSteps);
    filament_lookup_table[2][0] = bowdenLength.m_FSensorSteps;
    filament_lookup_table[2][1] = bowdenLength.m_FSensorSteps+10;
    filament_lookup_table[2][2] = bowdenLength.m_FSensorSteps;
    unload_filament_withSensor();
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
