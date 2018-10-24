#include "motion.h"
#include "shr16.h"
#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Arduino.h>
#include "main.h"
#include "mmctl.h"
#include "Buttons.h"
#include "permanent_storage.h"
#include "config.h"

// public variables:
int8_t filament_type[EXTRUDERS] = { -1, -1, -1, -1, -1};


// private constants:
// selector homes on the right end. afterwards it is moved to extruder 0
static const int SELECTOR_STEPS_AFTER_HOMING = -3650;  // reduced a tad to make better alignment
static const int IDLER_STEPS_AFTER_HOMING = -130;

static const int IDLER_FULL_TRAVEL_STEPS = 1420; // 16th micro steps
// after homing: 1420 into negative direction
// and 130 steps into positive direction

static const int SELECTOR_STEPS = 2790 / (EXTRUDERS - 1);
static const int IDLER_STEPS = 1420 / (EXTRUDERS - 1); // full travel = 1420 16th micro steps
static const int IDLER_PARKING_STEPS = (IDLER_STEPS / 2) + 40; // 40

static const int BOWDEN_LENGTH = 1000;
// endstop to tube  - 30 mm, 550 steps

static const int EJECT_PULLEY_STEPS = 2500;

// private variables:

static int selector_steps_for_eject = 0;

static int idler_steps_for_eject = 0;

// private functions:
static int set_idler_direction(int steps);
static int set_selector_direction(int steps);
static int set_pulley_direction(int steps);

bool checkOk();

void set_positions(int _current_extruder, int _next_extruder)
{
    // steps to move to new position of idler and selector
    int _selector_steps = ((_current_extruder - _next_extruder) * SELECTOR_STEPS) * -1;
    int _idler_steps = (_current_extruder - _next_extruder) * IDLER_STEPS;

    // move both to new position
    move_idler(_idler_steps); // remove this, when abs coordinates are implemented!
    move_selector(_selector_steps);
}

void reset_positions(uint8_t axis, int _current_extruder_pos, int _new_extruder_pos)
{
    // steps to move axis to new position of idler and selector independantly
    int steps = 0;
    switch(axis) {
      case AX_SEL:
        steps = ((_current_extruder_pos - _new_extruder_pos) * SELECTOR_STEPS) * -1;
        moveSmooth(AX_SEL, steps * -1, MAX_SPEED_SEL, false);        
        break;
      case AX_IDL:
        steps = ((_current_extruder_pos - _new_extruder_pos) * IDLER_STEPS);

        if (isIdlerParked) {
            steps - IDLER_PARKING_STEPS;
        }

        moveSmooth(AX_IDL, steps, MAX_SPEED_IDL, false);  // RMM:TODO handle failure!!!
        break;
    }
}

/**
 * @brief Eject Filament
 * move selector sideways and push filament forward little bit, so user can catch it,
 * unpark idler at the end to user can pull filament out
 * @param extruder: extruder channel (0..4)
 */
void eject_filament(int extruder)
{
    int selector_position = 0;

    int8_t selector_offset_for_eject = 0;
    int8_t idler_offset_for_eject = 0;

    // if there is still filament detected by PINDA unload it first
    if (isFilamentLoaded) {
        unload_filament_withSensor();
    }


    engage_filament_pulley(true);
    tmc2130_init_axis(AX_PUL, tmc2130_mode);


    // if we are want to eject fil 0-2, move seelctor to position 4 (right), if we want to eject filament 3 - 4, move
    // selector to position 0 (left)
    // maybe we can also move selector to service position in the future?
    if (extruder <= 2) {
        selector_position = 4;
    } else {
        selector_position = 0;
    }

    // count offset (number of positions) for desired selector and idler position for ejecting
    selector_offset_for_eject = active_extruder - selector_position;
    idler_offset_for_eject = active_extruder - extruder;

    // count number of desired steps for selector and idler and store it in static variable
    selector_steps_for_eject = (selector_offset_for_eject * SELECTOR_STEPS) * -1;
    idler_steps_for_eject = idler_offset_for_eject * IDLER_STEPS;

    // move selector and idler to new position
    move_idler(idler_steps_for_eject); // remove this, with when abs coordinates are implemented!
    move_selector(selector_steps_for_eject);

    // push filament forward
    move_pulley(EJECT_PULLEY_STEPS, 666);

    // unpark idler so user can easily remove filament
    engage_filament_pulley(false);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void recover_after_eject()
{
    // restore state before eject filament
    tmc2130_init_axis(AX_PUL, tmc2130_mode);


    // pull back filament
    engage_filament_pulley(true);
    move_pulley(-EJECT_PULLEY_STEPS);
    engage_filament_pulley(false);

    move_idler(-idler_steps_for_eject); // TODO 1: remove this, when abs coordinates are implemented!
    move_selector(-selector_steps_for_eject);

    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void load_filament_withSensor()
{

    engage_filament_pulley(
        true); // if idler is in parked position un-park him get in contact with filament
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    set_pulley_dir_push();

    int _loadSteps = 0;
    int _endstop_hit = 0;

    // load filament until FINDA senses end of the filament, means correctly loaded into the selector
    // we can expect something like 570 steps to get in sensor
    do {
        moveSmooth(AX_PUL, 1, 0, false);
        _loadSteps++;
        delayMicroseconds(5500);
    } while (isFilamentInFinda() == false && _loadSteps < 1500);

    // filament did not arrived at FINDA, let's try to correct that
    if (isFilamentInFinda() == false) {
        for (int i = 6; i > 0; i--) {
            if (isFilamentInFinda() == false) {
                // attempt to correct
                set_pulley_dir_pull();
                for (int i = 200; i >= 0; i--) {
                    moveSmooth(AX_PUL, 1, 0, false);
                    delayMicroseconds(1500);
                }

                set_pulley_dir_push();
                _loadSteps = 0;
                do {
                    moveSmooth(AX_PUL, 1, 0, false);
                    _loadSteps++;
                    delayMicroseconds(4000);
                    if (isFilamentInFinda()) {
                        _endstop_hit++;
                    }
                } while (_endstop_hit < 100 && _loadSteps < 500);
            }
        }
    }

    // still not at FINDA, error on loading, let's wait for user input
    if (isFilamentInFinda() == false) {
        bool _continue = false;
        bool _isOk = false;

        engage_filament_pulley(false);
        do {
            shr16_set_led(0x000);
            delay(800);
            if (!_isOk) {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            } else {
                shr16_set_led(1 << 2 * (4 - active_extruder));
                delay(100);
                shr16_set_led(2 << 2 * (4 - active_extruder));
                delay(100);
            }
            delay(800);

            switch (buttonClicked()) {
            case Btn::left:
                // just move filament little bit
                engage_filament_pulley(true);
                set_pulley_dir_push();

                for (int i = 0; i < 200; i++) {
                    moveSmooth(AX_PUL, 1, 0, false);
                    delayMicroseconds(5500);
                }
                engage_filament_pulley(false);
                break;
            case Btn::middle:
                // check if everything is ok
                engage_filament_pulley(true);
                _isOk = checkOk();
                engage_filament_pulley(false);
                break;
            case Btn::right:
                // continue with loading
                engage_filament_pulley(true);
                _isOk = checkOk();
                engage_filament_pulley(false);

                if (_isOk) { // there is no filament in finda any more, great!
                    _continue = true;
                }
                break;
            default:
                break;
            }

        } while (!_continue);

        engage_filament_pulley(true);
        // TODO: do not repeat same code, try to do it until succesfull load
        _loadSteps = 0;
        do {
            moveSmooth(AX_PUL, 1, 0, false);
            _loadSteps++;
            delayMicroseconds(5500);
        } while (isFilamentInFinda() == false && _loadSteps < 1500);
    }

    // RMM:TODO, changed to do acc in moveSmooth instead of moveSmooth(AX_PUL, 1, 0, false)
    const uint16_t steps = BowdenLength::get();

    switch (moveSmooth(AX_PUL, steps, MAX_SPEED_PUL, false)) {
      case MR_Success:
        tmc2130_disable_axis(AX_PUL, tmc2130_mode);
        isFilamentLoaded = true; // filament loaded
      case MR_Failed:
        tmc2130_disable_axis(AX_PUL, tmc2130_mode);
        // RMM:TODO2 Probably retract and try loading again once end seen
    }
}

/**
 * @brief unload_filament_withSensor
 * unloads filament from extruder - filament is above Bondtech gears
 */
void unload_filament_withSensor()
{
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    engage_filament_pulley(
        true); // if idler is in parked position un-park him get in contact with filament

    set_pulley_dir_pull();

    float _speed = 2000;
    float _first_point = 1800;
    float _second_point = 8700;
    int _endstop_hit = 0;

    // unload until FINDA senses end of the filament
    int _unloadSteps = 10000;
    do {
        moveSmooth(AX_PUL, 1, 0, false);
        _unloadSteps--;

        if (_unloadSteps < 1400 && _speed < 6000) {
            _speed = _speed + 3;
        }
        if (_unloadSteps < _first_point && _speed < 2500) {
            _speed = _speed + 2;
        }
        if (_unloadSteps < _second_point && _unloadSteps > 5000 && _speed > 550) {
            _speed = _speed - 2;
        }

        delayMicroseconds(_speed);
        if (isFilamentInFinda() == false && _unloadSteps < 2500) {
            _endstop_hit++;
        }

    } while (_endstop_hit < 50 && _unloadSteps > 0);  // RMM:TODO3 Reduced endstop hits from 100 step

    // move a little bit so it is not a grinded hole in filament
    for (int i = 100; i > 0; i--) {
        moveSmooth(AX_PUL, 1, 0, false);
        delayMicroseconds(5000);
    }

    // FINDA is still sensing filament, let's try to unload it once again
    if (isFilamentInFinda()) {
        for (int i = 6; i > 0; i--) {
            if (isFilamentInFinda()) {
                set_pulley_dir_push();
                for (int i = 150; i > 0; i--) {
                    moveSmooth(AX_PUL, 1, 0, false);
                    delayMicroseconds(4000);
                }

                set_pulley_dir_pull();
                int _steps = 4000;
                _endstop_hit = 0;
                do {
                    moveSmooth(AX_PUL, 1, 0, false);
                    _steps--;
                    delayMicroseconds(3000);
                    if (isFilamentInFinda() == false) {
                        _endstop_hit++;
                    }
                } while (_endstop_hit < 100 && _steps > 0);
            }
            delay(100);
        }
    }

    // error, wait for user input
    if (isFilamentInFinda()) {
        bool _continue = false;
        bool _isOk = false;

        engage_filament_pulley(false);
        do {
            shr16_set_led(0x000);
            delay(100);
            if (!_isOk) {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            } else {
                shr16_set_led(1 << 2 * (4 - active_extruder));
                delay(100);
                shr16_set_led(2 << 2 * (4 - active_extruder));
                delay(100);
            }
            delay(100);

            switch (buttonClicked()) {
            case Btn::left:
                // just move filament little bit
                engage_filament_pulley(true);
                set_pulley_dir_pull();

                for (int i = 0; i < 200; i++) {
                    moveSmooth(AX_PUL, 1, 0, false);
                    delayMicroseconds(5500);
                }
                engage_filament_pulley(false);
                break;
            case Btn::middle:
                // check if everything is ok
                engage_filament_pulley(true);
                _isOk = checkOk();
                engage_filament_pulley(false);
                break;
            case Btn::right:
                // continue with unloading
                engage_filament_pulley(true);
                _isOk = checkOk();
                engage_filament_pulley(false);

                if (_isOk) {
                    _continue = true;
                }
                break;
            default:
                break;
            }

        } while (!_continue);

        shr16_set_led(1 << 2 * (4 - previous_extruder));
    } else {
        // correct unloading
        _speed = 5000;
        // unload to PTFE tube
        set_pulley_dir_pull();
        for (int i = 450; i > 0; i--) { // 570
            moveSmooth(AX_PUL, 1, 0, false);
            delayMicroseconds(_speed);
        }
    }
    engage_filament_pulley(false);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);

    isFilamentLoaded = false; // filament unloaded
}

/**
 * @brief load_filament_intoExtruder
 * loads filament after confirmed by printer into the Bontech
 * pulley gears so they can grab them.
 * We reduce here stepwise the motor current, to prevent grinding into the
 * filament as good as possible.
 *
 * TODO 1: this procedure is most important for high reliability.
 * The speed must be set accordingly to the settings in the slicer
 */
void load_filament_into_extruder()
{
    uint8_t current_running_normal[3] = CURRENT_RUNNING_NORMAL;
    uint8_t current_running_stealth[3] = CURRENT_RUNNING_STEALTH;
    uint8_t current_holding_normal[3] = CURRENT_HOLDING_NORMAL;
    uint8_t current_holding_stealth[3] = CURRENT_HOLDING_STEALTH;

    engage_filament_pulley(
        true); // if idler is in parked position un-park him get in contact with filament
    set_pulley_dir_push();

    // #### PLA ####
    // set current to 100%
    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    move_pulley(151, 385);


    // set current to 75%
    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                         current_running_normal[AX_PUL] - (current_running_normal[AX_PUL] / 4) );
    } else {
        tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL],
                                          current_running_stealth[AX_PUL] - (current_running_stealth[AX_PUL] / 4) );
    }
    move_pulley(170, 385);

    // set current to 25%
    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                         current_running_normal[AX_PUL] / 4);
    } else {
        tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL],
                                          current_running_stealth[AX_PUL] / 4);
    }
    move_pulley(451, 455);   // RMM:TODO4 May be able to sync with printer longer here

    engage_filament_pulley(false);
    tmc2130_disable_axis(AX_PUL, tmc2130_mode);
}

void init_Pulley()
{
    float _speed = 3000;

    // TODO 1: replace with move-commands

    set_pulley_dir_push();
    for (int i = 50; i > 0; i--) {
        moveSmooth(AX_PUL, 1, 0, false);
        delayMicroseconds(_speed);
        shr16_set_led(1 << 2 * (int)(i / 50)); // TODO 2: What the heck?
    }

    set_pulley_dir_pull();
    for (int i = 50; i > 0; i--) {
        moveSmooth(AX_PUL, 1, 0, false);
        delayMicroseconds(_speed);
        shr16_set_led(1 << 2 * (4 - (int)(i / 50))); // TODO 2: What the heck?
    }
}

/**
 * @brief engage_filament_pulley
 * Turns the idler drum to engage or disengage the filament pully
 * @param engage
 * If true, pully can drive the filament afterwards
 * if false, idler will be parked, so the filament can move freely
 */
void engage_filament_pulley(bool engage)
{
    if (isIdlerParked && engage) { // get idler in contact with filament
        move_idler(IDLER_PARKING_STEPS);
        isIdlerParked = false;
    } else if (!isIdlerParked && !engage) { // park idler so filament can move freely
        move_idler(IDLER_PARKING_STEPS * -1);
        isIdlerParked = true;
    }
}

void home()
{
    homeIdlerSmooth();
    homeSelectorSmooth();
    shr16_set_led(0x155);

    active_extruder = 0;

    engage_filament_pulley(false);
    shr16_set_led(0x000);

    isFilamentLoaded = false;
    shr16_set_led(1 << 2 * (4 - active_extruder));

    isHomed = true;
}

void move_idler(int steps, uint16_t speed)
{
    if (speed > MAX_SPEED_IDL) {
        speed = MAX_SPEED_IDL;
    }
    moveSmooth(AX_IDL, steps, MAX_SPEED_IDL);
}

/**
 * @brief move_selector
 * Strictly prevent selector movement, when filament is in FINDA
 * @param steps, number of micro steps
 */
void move_selector(int steps, uint16_t speed)
{
    if (speed > MAX_SPEED_SEL) {
        speed = MAX_SPEED_SEL;
    }
    if (tmc2130_mode == STEALTH_MODE) {
        if (speed > MAX_SPEED_STEALTH_SEL) {
            speed = MAX_SPEED_STEALTH_SEL;
        }
    }
    if (isFilamentInFinda() == false) {
        moveSmooth(AX_SEL, steps, speed);
    }
}

void move_pulley(int steps, uint16_t speed)
{
    moveSmooth(AX_PUL, steps, speed, false, true);
}

/**
 * @brief set_idler_direction
 * @param steps: positive = towards engaging filament nr 1,
 * negative = towards engaging filament nr 5.
 * @return abs(steps)
 */
int set_idler_direction(int steps)
{
    if (steps < 0) {
        steps = steps * -1;
        shr16_set_dir(shr16_get_dir() & ~4);
    } else {
        shr16_set_dir(shr16_get_dir() | 4);
    }
    return steps;
}

/**
 * @brief set_selector_direction
 * Sets the direction bit on the motor driver and returns positive number of steps
 * @param steps: positive = to the right (towards filament 5),
 * negative = to the left (towards filament 1)
 * @return abs(steps)
 */
int set_selector_direction(int steps)
{
    if (steps < 0) {
        steps = steps * -1;
        shr16_set_dir(shr16_get_dir() & ~2);
    } else {
        shr16_set_dir(shr16_get_dir() | 2);
    }
    return steps;
}

/**
 * @brief set_pulley_direction
 * @param steps, positive (push) or negative (pull)
 * @return abs(steps)
 */
int set_pulley_direction(int steps)
{
    if (steps < 0) {
        steps = steps * -1;
        set_pulley_dir_pull();
    } else {
        set_pulley_dir_push();
    }
    return steps;
}

void set_pulley_dir_push()
{
    shr16_set_dir(shr16_get_dir() & ~1);
}
void set_pulley_dir_pull()
{
    shr16_set_dir(shr16_get_dir() | 1);
}

bool checkOk()
{
    bool _ret = false;
    int _steps = 0;
    int _endstop_hit = 0;

    // filament in FINDA, let's try to unload it
    set_pulley_dir_pull();
    if (isFilamentInFinda()) {
        _steps = 3000;
        _endstop_hit = 0;
        do {
            moveSmooth(AX_PUL, 1, 0, false);
            delayMicroseconds(3000);
            if (isFilamentInFinda() == false) {
                _endstop_hit++;
            }
            _steps--;
        } while (_steps > 0 && _endstop_hit < 50);
    }

    if (isFilamentInFinda() == false) {
        // looks ok, load filament to FINDA
        set_pulley_dir_push();

        _steps = 3000;
        _endstop_hit = 0;
        do {
            moveSmooth(AX_PUL, 1, 0, false);
            delayMicroseconds(3000);
            if (isFilamentInFinda()) {
                _endstop_hit++;
            }
            _steps--;
        } while (_steps > 0 && _endstop_hit < 50);

        if (_steps == 0) {
            // we ran out of steps, means something is again wrong, abort
            _ret = false;
        } else {
            // looks ok !
            // unload to PTFE tube
            set_pulley_dir_pull();
            for (int i = 600; i > 0; i--) { // 570
                moveSmooth(AX_PUL, 1, 0, false);
                delayMicroseconds(3000);
            }
            _ret = true;
        }

    } else {
        // something is wrong, abort
        _ret = false;
    }

    return _ret;
}

MotReturn homeSelectorSmooth()
{
    for (int c = 2; c > 0; c--) { // touch end 2 times
        moveSmooth(AX_SEL, 4000, 2000,
                   false); // 3000 is too fast, 2500 works, decreased to 2000 for production
        if (c > 1) {
            moveSmooth(AX_SEL, -200, 2000, false);
        }
    }

    return moveSmooth(AX_SEL, SELECTOR_STEPS_AFTER_HOMING, 8000, false);
}

MotReturn homeIdlerSmooth()
{
    for (int c = 2; c > 0; c--) { // touch end 2 times
        moveSmooth(AX_IDL, 2000, 3000, false);
        if (c > 1) {
            moveSmooth(AX_IDL, -300, 5000, false); // move a bit back  #### I think this isn't in line with coordanance in FW
        }
    }

    return moveSmooth(AX_IDL, IDLER_STEPS_AFTER_HOMING, MAX_SPEED_IDL, false, false);
}

/**
 * @brief moveTest
 * @param axis, index of axis, use AX_PUL, AX_SEL or AX_IDL
 * @param steps, number of micro steps to move
 * @param speed, max. speed
 * @param rehomeOnFail: flag, by default true, set to false
 *   in homing commands, to prevent endless loops and stack overflow.
 * @return
 */
// TODO 3: compensate delay for computation time, to get accurate speeds
// TODO 3: add callback or another parameter, which can stop the motion
// (e.g. for testing FINDA, timeout, soft stall guard limits, push buttons...)
MotReturn moveSmooth(uint8_t axis, int steps, int speed,
                     bool rehomeOnFail, bool withStallDetection)
{
    MotReturn ret = MR_Success;

    if (tmc2130_mode == STEALTH_MODE) {
        withStallDetection = false;
    }

    shr16_set_led(0);

    float vMax = speed;
    float acc = ACC_NORMAL; // Note: tested selector successfully with 100k
    if (tmc2130_mode == STEALTH_MODE) {
        acc = ACC_STEALTH;
    }
    float v0 = 200; // steps/s, minimum speed
    float v = v0; // current speed
    int accSteps = 0; // number of steps for acceleration
    int stepsDone = 0;
    int stepsLeft = 0;

    switch (axis) {
    case AX_PUL:
        stepsLeft = set_pulley_direction(steps);
        break;
    case AX_IDL:
        stepsLeft = set_idler_direction(steps);
        break;
    case AX_SEL:
        stepsLeft = set_selector_direction(steps);
        break;
    }

    enum State {
        Accelerate = 0,
        ConstVelocity = 1,
        Decelerate = 2,
    };

    State st = Accelerate;
    shr16_set_led(1 << 0);

    v = v0;

    while (stepsLeft) {
        switch (axis) {
        case AX_PUL:
            PIN_STP_PUL_HIGH;
            PIN_STP_PUL_LOW;
            if (withStallDetection && digitalRead(A3) == 1) { // stall detected
                delay(50); // delay to release the stall detection
                return MR_Failed;
            }
            break;
        case AX_IDL:
            PIN_STP_IDL_HIGH;
            PIN_STP_IDL_LOW;
            if (withStallDetection && digitalRead(A5) == 1) { // stall detected
                delay(50); // delay to release the stall detection
                if (rehomeOnFail) {
                    if (homeIdlerSmooth() == MR_Success) {
                        reset_positions(AX_IDL, 0, active_extruder);
                        return MR_FailedAndRehomed;
                    } else {
                        return MR_Failed;
                    }
                } else {
                    return MR_Failed;
                }
            }
            break;
        case AX_SEL:
            PIN_STP_SEL_HIGH;
            PIN_STP_SEL_LOW;
            if (withStallDetection && digitalRead(A4) == 1) { // stall detected
                delay(50); // delay to release the stall detection
                if (rehomeOnFail) {
                    if (homeSelectorSmooth() == MR_Success) {
                        reset_positions(AX_SEL, 0, active_extruder);
                        return MR_FailedAndRehomed;
                    } else {
                        return MR_Failed;
                    }
                } else {
                    return MR_Failed;
                }
            }
            break;
        }

        stepsDone++;
        stepsLeft--;

        float dt = 1 / v;
        delayMicroseconds(1e6 * dt);

        switch (st) {
        case Accelerate:
            v += acc * dt;
            if (v >= vMax) {
                accSteps = stepsDone;
                st = ConstVelocity;
                shr16_set_led(1 << 2);

                v = vMax;
            } else if (stepsDone > stepsLeft) {
                accSteps = stepsDone;
                st = Decelerate;
                shr16_set_led(1 << 4);

            }
            break;
        case ConstVelocity: {
            volatile float dummy = acc * dt; // keep same timing in branches of switch
            if (stepsLeft <= accSteps) {
                st = Decelerate;
                shr16_set_led(1 << 4);
            }
        }
        break;
        case Decelerate: {
            v -= acc * dt;
            if (v < v0) {
                v = v0;
            }
        }
        break;
        }
    }

    return ret;
}

bool isFilamentInFinda()
{
    return digitalRead(A1) == 1;
}
