#include "motion.h"
#include "shr16.h"
#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Arduino.h>
#include "main.h"
#include "uart.h"
#include "mmctl.h"
#include "Buttons.h"
#include "permanent_storage.h"
#include "config.h"

// public variables:
BowdenLength bowdenLength;
uint16_t BOWDEN_LENGTH = bowdenLength.get();
int16_t tempFSensorSteps = bowdenLength.getFSensorSteps();

int8_t filament_type[EXTRUDERS] = { 0, 0, 0, 0, 0};
int filament_lookup_table[9][3] =
{{TYPE_0_MAX_SPPED_PUL,               TYPE_1_MAX_SPPED_PUL,               TYPE_2_MAX_SPPED_PUL},
 {TYPE_0_ACC_FEED_PUL,                TYPE_1_ACC_FEED_PUL,                TYPE_2_ACC_FEED_PUL},
 {tempFSensorSteps,                   tempFSensorSteps+10,                tempFSensorSteps},
 {TYPE_0_FILAMENT_PARKING_STEPS,      TYPE_1_FILAMENT_PARKING_STEPS,      TYPE_2_FILAMENT_PARKING_STEPS},
 {TYPE_0_FSensor_Sense_STEPS,         TYPE_1_FSensor_Sense_STEPS,         TYPE_2_FSensor_Sense_STEPS},
 {TYPE_0_FEED_SPEED_PUL,              TYPE_1_FEED_SPEED_PUL,              TYPE_2_FEED_SPEED_PUL},
 {TYPE_0_L2ExStageOne,                TYPE_1_L2ExStageOne,                TYPE_2_L2ExStageOne},
 {TYPE_0_L2ExStageTwo,                TYPE_1_L2ExStageTwo,                TYPE_2_L2ExStageTwo},
 {TYPE_0_UnloadSpeed,                 TYPE_1_UnloadSpeed,                 TYPE_2_UnloadSpeed}
};

// private constants:
const uint8_t IDLER_PARKING_STEPS = (355 / 2) + 40;      // 217
const uint16_t EJECT_PULLEY_STEPS = 2000;

const int selectorStepPositionsFromHome[EXTRUDERS+2] = {-3700, -3000, -2300, -1600, -900, -100, 0};
const int idlerStepPositionsFromHome[EXTRUDERS+1] = {-130, -485, -840, -1195, -1550, 0};

uint8_t selSGFailCount = 0;
uint8_t idlSGFailCount = 0;

// private functions:
static uint16_t set_idler_direction(int steps);
static uint16_t set_selector_direction(int steps);
static uint16_t set_pulley_direction(int steps);

bool set_positions(uint8_t _next_extruder, bool update_extruders)
{
    bool _return0 = false, _return1 = false;
    if (update_extruders) {
        previous_extruder = active_extruder;
        active_extruder = _next_extruder;
        FilamentLoaded::set(active_extruder);
        unsigned char temp[3] = {'A', 'E', (uint8_t)active_extruder};
        txPayload(temp);
    }
    if (!isHomed) home(true);
    else {
        _return0 = steps2setIDL2pos(_next_extruder);
        _return1 = steps2setSEL2pos(_next_extruder);
    }
    if (!_return0 || !_return1) return false;
    else return true;
}

bool steps2setIDL2pos(uint8_t _next_extruder)
{
    bool _return = false;
    if (_next_extruder == EXTRUDERS) _next_extruder -= 1;
    int _idler_steps = (idlerStepPositionsFromHome[_next_extruder] - idlerStepPositionsFromHome[activeIdlPos]);
    if (move_idler(_idler_steps)) { activeIdlPos = _next_extruder; _return = true; }
    return _return;
}

bool steps2setSEL2pos(uint8_t _next_extruder)
{
    bool _return = false;
    int _selector_steps = (selectorStepPositionsFromHome[_next_extruder] - selectorStepPositionsFromHome[activeSelPos]);
    if (move_selector(_selector_steps)) { activeSelPos = _next_extruder; _return = true; }
    return _return;
}

void set_idler_toLast_positions(uint8_t _next_extruder)
{
    bool previouslyEngaged = !isIdlerParked;
    homeIdlerSmooth();
    if (!steps2setIDL2pos(_next_extruder)) fixTheProblem();
    engage_filament_pulley(previouslyEngaged);
}

void set_sel_toLast_positions(uint8_t _next_extruder)
{
    homeSelectorSmooth();
    if (!steps2setSEL2pos(_next_extruder)) fixTheProblem();
}

/**
 * @brief Eject Filament
 * move selector sideways and push filament forward little bit, so user can catch it,
 * unpark idler at the end to user can pull filament out
 * @param extruder: extruder channel (0..4)
 */
void eject_filament(uint8_t extruder)
{
    // if there is still filament detected by PINDA unload it first
    if (isFilamentLoaded()) {
        unload_filament_withSensor();
    }

    set_positions(extruder, true);
    if (active_extruder == (EXTRUDERS - 1)) steps2setSEL2pos(0);
    else steps2setSEL2pos(EXTRUDERS);
    isEjected = true;

    engage_filament_pulley(true);
    tmc2130_init_axis(AX_PUL, tmc2130_mode);

    // push filament forward
    move_pulley(EJECT_PULLEY_STEPS, filament_lookup_table[5][filament_type[active_extruder]]);

    // unpark idler so user can easily remove filament
    engage_filament_pulley(false);
    shr16_clr_ena(AX_PUL);
}

void recover_after_eject()
{
    while (isFilamentLoaded()) fixTheProblem();
    home(true);
    isEjected = false;
}

bool load_filament_withSensor(uint16_t setupBowLen)
{
    uint8_t retries = 1;
loop:
    {
        if (!isHomed && (setupBowLen == 0)) home(true);
        engage_filament_pulley(true); // get in contact with filament
        tmc2130_init_axis(AX_PUL, tmc2130_mode);

        // load filament until FINDA senses end of the filament, means correctly loaded into the selector
        // we can expect something like 570 steps to get in sensor, try 1000 incase user is feeding to pulley

        if (moveSmooth(AX_PUL, 2000, filament_lookup_table[5][filament_type[active_extruder]],
                       false, false, ACC_NORMAL, true) == MR_Success) { // Move to Pulley
            if (setupBowLen != 0) moveSmooth(AX_PUL, setupBowLen, filament_lookup_table[0][filament_type[active_extruder]],
                                                 false, false, filament_lookup_table[1][filament_type[active_extruder]]);      // Load filament down to MK3-FSensor
            else {
                moveSmooth(AX_PUL, 500, filament_lookup_table[5][filament_type[active_extruder]],
                           false, false);
                moveSmooth(AX_PUL, BOWDEN_LENGTH - 500, filament_lookup_table[0][filament_type[active_extruder]],
                           false, false, filament_lookup_table[1][filament_type[active_extruder]]);      // Load filament down to near MK3-FSensor                
                uint8_t iLoop2 = 0;
            loop2:
                txRESEND         = false;
                startRxFlag      = false;
                pendingACK       = false;
                txPayload((unsigned char*)"FS-");  // 'FS-' Starting FSensor checking on MK3
                fsensor_triggered = false;

                if (moveSmooth(AX_PUL, filament_lookup_table[4][filament_type[active_extruder]], 200,
                    false, false, ACC_NORMAL, false, true) == MR_Success) {
                    moveSmooth(AX_PUL, filament_lookup_table[2][filament_type[active_extruder]],
                    filament_lookup_table[5][filament_type[active_extruder]] * 0.8, false, false);   // Load from MK3-FSensor to Bontech gears, ready for loading into extruder with C0 command
                } else {
                    if (iLoop2 < 3) { // 4 attempts
                        delay(50);
                        moveSmooth(AX_PUL, ((filament_lookup_table[4][filament_type[active_extruder]]) * -1),
                                   filament_lookup_table[5][filament_type[previous_extruder]],
                                   false, false, ACC_NORMAL, true);
                        delay(50);
                        iLoop2++;
                        goto loop2;
                    } else {
                        txPayload((unsigned char*)"ZL2"); // Report Loading failed to MK3
                        fixTheProblem();
                        goto loop;
                    }
                }
            }
            shr16_clr_led(); //shr16_set_led(0x000);                                                 // Clear all 10 LEDs on MMU unit
            shr16_set_led(1 << 2 * (4 - active_extruder));
            mmuFSensorLoading = false;
            return true;
        }
        if (retries > 0) { set_idler_toLast_positions(active_extruder); retries = 0; goto loop; }
        txPayload((unsigned char*)"ZL1"); // Report Loading failed to MK3
        fixTheProblem();
        goto loop;
    }
    startWakeTime = millis();  // Start/Reset wakeTimer
}

/**
 * @brief unload_filament_withSensor
 * unloads filament from extruder - filament is above Bondtech gears
 */
bool unload_filament_withSensor(uint8_t extruder)
{
    if (isFilamentLoaded()) {
        tmc2130_init_axis(AX_PUL, tmc2130_mode);
        tmc2130_init_axis(AX_IDL, tmc2130_mode);
        
        engage_filament_pulley(true); // get in contact with filament
        uint8_t mmPerSecSpeedUpper = (0xFF & ((filament_lookup_table[8][filament_type[active_extruder]] / AX_PUL_STEP_MM_Ratio) >> 8));
        uint8_t mmPerSecSpeedLower = (0xFF & (filament_lookup_table[8][filament_type[active_extruder]] / AX_PUL_STEP_MM_Ratio));
        unsigned char txUFR[3] = {'U',mmPerSecSpeedUpper, mmPerSecSpeedLower};
        txPayload(txUFR);
        delay(40);
        moveSmooth(AX_PUL, -(100*AX_PUL_STEP_MM_Ratio), filament_lookup_table[8][filament_type[active_extruder]],
                   false, false, ACC_NORMAL);
        if (moveSmooth(AX_PUL, (BOWDEN_LENGTH * -1),
                   filament_lookup_table[0][filament_type[extruder]], false, false,
                   filament_lookup_table[1][filament_type[extruder]], true) == MR_Success) goto loop;
        if (moveSmooth(AX_PUL, -3000, filament_lookup_table[5][filament_type[extruder]],
                       false, false, ACC_NORMAL, true) == MR_Success) {                                                  // move to trigger FINDA
            loop:
            moveSmooth(AX_PUL, filament_lookup_table[3][filament_type[extruder]],
                       filament_lookup_table[5][filament_type[extruder]], false, false, ACC_NORMAL);                     // move to filament parking position
        } else if (isFilamentLoaded()) {
            txPayload((unsigned char*)"ZU-"); // Report Unloading failed to MK3
            if (extruder != active_extruder) fixTheProblem(true);
            else fixTheProblem();
            homedOnUnload = true;
        }
    }
    
    shr16_clr_ena(AX_PUL);
    engage_filament_pulley(false);
    return true;
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

    engage_filament_pulley(true); // get in contact with filament

    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    move_pulley(150, filament_lookup_table[6][filament_type[active_extruder]]);

    // set current to 75%
    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                         current_running_normal[AX_PUL] - (current_running_normal[AX_PUL] / 4), false);
    } else {
        tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL],
                                          current_running_stealth[AX_PUL] - (current_running_stealth[AX_PUL] / 4));
    }
    move_pulley(170, filament_lookup_table[6][filament_type[active_extruder]]);

    // set current to 25%
    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                         current_running_normal[AX_PUL] / 4, false);
    } else {
        tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL],
                                          current_running_stealth[AX_PUL] / 4);
    }
    move_pulley(650, filament_lookup_table[7][filament_type[active_extruder]]);
    shr16_clr_ena(AX_PUL);
    engage_filament_pulley(false); // release contact with filament

    // reset currents
    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                         current_running_normal[AX_PUL], false);
    } else {
        tmc2130_init_axis_current_stealth(AX_PUL, current_holding_stealth[AX_PUL],
                                          current_running_stealth[AX_PUL]);
    }
    shr16_clr_ena(AX_PUL);
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
    uint8_t current_running_normal[3] = CURRENT_RUNNING_NORMAL;
    uint8_t current_holding_normal[3] = CURRENT_HOLDING_NORMAL;
    uint8_t current_holding_loading[3] = CURRENT_HOLDING_NORMAL_LOADING;

    if (isIdlerParked && engage) { // get idler in contact with filament
        tmc2130_init_axis_current_normal(AX_IDL, current_holding_loading[AX_IDL],
                                         current_running_normal[AX_IDL], false);
        move_idler(IDLER_PARKING_STEPS);
        isIdlerParked = false;
    } else if (!isIdlerParked && !engage) { // park idler so filament can move freely
        move_idler(IDLER_PARKING_STEPS * -1);
        tmc2130_init_axis_current_normal(AX_IDL, current_holding_normal[AX_IDL],
                                         current_running_normal[AX_IDL], false);
        isIdlerParked = true;
    }
}

void home(bool doToolSync)
{
    bool previouslyEngaged = !isIdlerParked;
    homeIdlerSmooth();
    homeSelectorSmooth();

    shr16_clr_led();
    shr16_set_led(0x155);       // All five red

    shr16_clr_led();            // All five off

    shr16_clr_led();
    shr16_set_led(1 << 2 * (4 - active_extruder));

    isHomed = true;
    startWakeTime = millis();

    if (doToolSync) {
        set_positions(active_extruder);
        FilamentLoaded::set(active_extruder);
        engage_filament_pulley(previouslyEngaged);
        trackToolChanges = 0;
    }
}

bool move_idler(int steps, uint16_t speed)
{
    if (speed > MAX_SPEED_IDL) {
        speed = MAX_SPEED_IDL;
    }

    if (moveSmooth(AX_IDL, steps, MAX_SPEED_IDL, true, true, ACC_IDL_NORMAL) == MR_Failed)
         return false;
    else return true;
}

/**
 * @brief move_selector
 * Strictly prevent selector movement, when filament is in FINDA
 * @param steps, number of micro steps
 */
bool move_selector(int steps, uint16_t speed)
{
    if (speed > MAX_SPEED_SEL) {
        speed = MAX_SPEED_SEL;
    }
    if (tmc2130_mode == STEALTH_MODE) {
        if (speed > MAX_SPEED_STEALTH_SEL) {
            speed = MAX_SPEED_STEALTH_SEL;
        }
    }
    if (!isFilamentLoaded()) {
        if (moveSmooth(AX_SEL, steps, speed) == MR_Failed)
           return false;
    } else return false;
           return true;
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
uint16_t set_idler_direction(int steps)
{
    if (steps < 0) {
        steps = steps * -1;
        shr16_write(shr16_v & ~SHR16_DIR_IDL);
    } else {
        shr16_write(shr16_v | SHR16_DIR_IDL);
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
uint16_t set_selector_direction(int steps)
{
    if (steps < 0) {
        steps = steps * -1;
        shr16_write(shr16_v & ~SHR16_DIR_SEL);
    } else {
        shr16_write(shr16_v | SHR16_DIR_SEL);
    }
    return steps;
}

/**
 * @brief set_pulley_direction
 * @param steps, positive (push) or negative (pull)
 * @return abs(steps)
 */
uint16_t set_pulley_direction(int steps)
{
    if (steps < 0) {
        steps = steps * -1;
        shr16_write(shr16_v | SHR16_DIR_PUL);
    } else {
        shr16_write(shr16_v & ~SHR16_DIR_PUL);
    }
    return steps;
}

MotReturn homeSelectorSmooth()
{
    for (int c = 2; c > 0; c--) { // touch end 2 times
        tmc2130_init(HOMING_MODE);  // trinamic, homing
        moveSmooth(AX_SEL, 4000, 2000, false);
        tmc2130_init(tmc2130_mode);  // trinamic, normal
        if (c > 1) moveSmooth(AX_SEL, -300, 2000, false);
    }
    tmc2130_init(tmc2130_mode);  // trinamic, homing
    activeSelPos = EXTRUDERS+1;
    return MR_Success;
}

MotReturn homeIdlerSmooth(bool toLastFilament)
{
    tmc2130_init(tmc2130_mode);  // trinamic, normal
    moveSmooth(AX_IDL, -250, MAX_SPEED_IDL, false);
    for (uint8_t c = 2; c > 0; c--) { // touch end 2 times
        tmc2130_init(HOMING_MODE);  // trinamic, homing
        moveSmooth(AX_IDL, 2600, 6000, false, true, 80000);
        tmc2130_init(tmc2130_mode);  // trinamic, homing
        if (c > 1) moveSmooth(AX_IDL, -200, MAX_SPEED_IDL, false, true, ACC_IDL_NORMAL);
        delay(50);
    }
    isIdlerParked = false;
    activeIdlPos = EXTRUDERS;
    if (toLastFilament) {
        uint8_t filament = 0;
        FilamentLoaded::get(filament);
        active_extruder = filament;
        unsigned char temp[3] = {'A', 'E', (uint8_t)active_extruder};
        txPayload(temp);
        steps2setIDL2pos(active_extruder);
        engage_filament_pulley(false);
    }
    return MR_Success;
}

/**
 * @brief disableSteppers
 */
void disableAllSteppers(void)
{
    shr16_clr_ena_all();
    isHomed = false;
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
MotReturn moveSmooth(uint8_t axis, int steps, int speed, bool rehomeOnFail,
                     bool withStallDetection, float acc,
                     bool withFindaDetection, bool withFSensorDetection)
{
    shr16_set_ena(axis);
    startWakeTime = millis();
    MotReturn ret = MR_Success;
    if (withFindaDetection or withFSensorDetection) ret = MR_Failed;

    if (tmc2130_mode == STEALTH_MODE) {
        withStallDetection = false;
    }

    float vMax = speed;
    float v0 = 200; // steps/s, minimum speed
    float v = v0; // current speed
    int accSteps = 0; // number of steps for acceleration
    int stepsDone = 0;
    int stepsLeft = 0;

    switch (axis) {
    case AX_PUL:
        stepsLeft = set_pulley_direction(steps);
        tmc2130_init_axis(AX_PUL, tmc2130_mode);
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

    while (stepsLeft) {
        switch (axis) {
        case AX_PUL:
            PIN_STP_PUL_HIGH;
            PIN_STP_PUL_LOW;
            if (withStallDetection && digitalRead(A3)) { // stall detected
                delay(50); // delay to release the stall detection
                return MR_Failed;
            }
            if (withFindaDetection && ( steps > 0 ) && isFilamentLoaded()) return MR_Success;
            if (withFindaDetection && ( steps < 0 ) && (isFilamentLoaded() == false)) return MR_Success;
            if (withFSensorDetection && fsensor_triggered) {
                txACK();      // Send  ACK Byte
                fsensor_triggered = false;
                return MR_Success;
            }
            break;
        case AX_IDL:
            PIN_STP_IDL_HIGH;
            PIN_STP_IDL_LOW;
            if (withStallDetection && digitalRead(A5)) { // stall detected
                delay(50); // delay to release the stall detection
                if (rehomeOnFail) {
                    if (idlSGFailCount < 3) {
                        idlSGFailCount++;
                        set_idler_toLast_positions(active_extruder);
                        return MR_FailedAndRehomed;
                    } else {
                      fixIdlCrash();
                      return MR_FailedAndRehomed;
                    }
                } else return MR_Failed;
            }
            break;
        case AX_SEL:
            PIN_STP_SEL_HIGH;
            PIN_STP_SEL_LOW;
            if (withStallDetection && digitalRead(A4)) { // stall detected
                delay(50); // delay to release the stall detection
                if (rehomeOnFail) {
                    if (selSGFailCount < 3) {
                        selSGFailCount++;
                        set_sel_toLast_positions(active_extruder);
                        return MR_FailedAndRehomed;
                    } else {
                      fixSelCrash();
                      return MR_FailedAndRehomed;
                    }
                } else return MR_Failed;
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

                v = vMax;
            } else if (stepsDone > stepsLeft) {
                accSteps = stepsDone;
                st = Decelerate;

            }
            break;
        case ConstVelocity: {
            if (stepsLeft <= accSteps) {
                st = Decelerate;
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
    switch (axis) {
    case AX_IDL:
        idlSGFailCount = 0;
        break;
    case AX_SEL:
        selSGFailCount = 0;
        break;
    }
    return ret;
}
