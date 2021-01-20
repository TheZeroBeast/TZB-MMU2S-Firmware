#include "motion.h"
#include "shr16.h"
#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Arduino.h>
#include "main.h"
#include "uart.h"
#include "Buttons.h"

// public variables:
BowdenLength bowdenLength;
uint16_t BOWDEN_LENGTH = bowdenLength.get();
uint16_t MAX_SPEED_SELECTOR =  MAX_SPEED_SEL_DEF_STEALTH; // micro steps
uint16_t MAX_SPEED_IDLER    =  MAX_SPEED_IDL_DEF_STEALTH; // micro steps
uint32_t GLOBAL_ACC         =  GLOBAL_ACC_DEF_STEALTH; // micro steps / s²
int8_t filament_type[EXTRUDERS] = { 0, 0, 0, 0, 0};
int filament_lookup_table[9][3] =
{{TYPE_0_MAX_SPPED_PUL,               TYPE_1_MAX_SPPED_PUL,               TYPE_2_MAX_SPPED_PUL},
 {TYPE_0_ACC_FEED_PUL,                TYPE_1_ACC_FEED_PUL,                TYPE_2_ACC_FEED_PUL},
 {0,                                  0,                                  0},  // Not used with IR_SENSOR
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

const int selectorStepPositionsFromHome[EXTRUDERS+2] = {-3700, -3002, -2305, -1607, -910, -100, 0};
const int idlerStepPositionsFromHome[EXTRUDERS+1] = {-130, -485, -840, -1195, -1550, 0};

uint8_t selSGFailCount = 0;
uint8_t idlSGFailCount = 0;

// private functions:
static uint16_t set_idler_direction(int steps);
static uint16_t set_selector_direction(int steps);
static uint16_t set_pulley_direction(int steps);

/**
 * @brief move_idler
 * @param steps, number of micro steps
 */
bool move_idler(int steps, uint16_t speed)
{
    bool ret, isLoadingBackup = isLoading;
    isLoading = true;
    tmc2130_init(tmc2130_mode);
    if (speed > MAX_SPEED_IDLER) speed = MAX_SPEED_IDLER;
    if (moveSmooth(AX_IDL, steps, speed, true, true, GLOBAL_ACC) == MR_Failed) ret = false;
    else ret = true;
    isLoading = isLoadingBackup;
    tmc2130_init(tmc2130_mode);
    return ret;
}

/**
 * @brief move_selector
 * Strictly prevent selector movement, when filament is in FINDA
 * @param steps, number of micro steps
 */
bool move_selector(int steps, uint16_t speed)
{
    if (speed > MAX_SPEED_SELECTOR) speed = MAX_SPEED_SELECTOR;
    if (!isFilamentLoaded()) {
        if (moveSmooth(AX_SEL, steps, speed) == MR_Failed) return false;
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

/**
 * @brief disableSteppers
 */
void disableAllSteppers(void)
{
    shr16_clr_ena_all();
    isHomed = false;
}

MotReturn homeSelectorSmooth()
{
    uint32_t acc_backup = GLOBAL_ACC;
    for (int c = 2; c > 0; c--) { // touch end 2 times
        tmc2130_init(HOMING_MODE);  // trinamic, homing
        GLOBAL_ACC = GLOBAL_ACC_DEF_NORMAL;
        moveSmooth(AX_SEL, 4000, 2000, false);
        GLOBAL_ACC = acc_backup;
        tmc2130_init(tmc2130_mode);  // trinamic, normal
        if (c > 1) moveSmooth(AX_SEL, -300, 2000, false);
    }
    tmc2130_init(tmc2130_mode);  // trinamic, homing
    activeSelPos = EXTRUDERS+1;
    return MR_Success;
}

MotReturn homeIdlerSmooth(bool toLastFilament)
{
    uint32_t acc_backup = GLOBAL_ACC;
    tmc2130_init(tmc2130_mode);  // trinamic, normal
    moveSmooth(AX_IDL, -250, MAX_SPEED_IDLER, false);
    for (uint8_t c = 2; c > 0; c--) { // touch end 2 times
        tmc2130_init(HOMING_MODE);  // trinamic, homing
        GLOBAL_ACC = GLOBAL_ACC_DEF_NORMAL;
        moveSmooth(AX_IDL, 2600, 6350, false, true);
        tmc2130_init(tmc2130_mode);  // trinamic, homing
        GLOBAL_ACC = acc_backup;
        if (c > 1) moveSmooth(AX_IDL, -600, MAX_SPEED_IDLER, false);
        delay(50);
    }
    isIdlerParked = false;
    activeIdlPos = EXTRUDERS;
    if (toLastFilament) {
        uint8_t filament = 0;
        FilamentLoaded::get(filament);
        active_extruder = filament;
        setIDL2pos(active_extruder);
        engage_filament_pulley(false);
    }
    return MR_Success;
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
                     bool withFindaDetection, bool withIR_SENSORDetection)
{
    // if in stealth mode don't look for stallguard
    if (tmc2130_mode == STEALTH_MODE) rehomeOnFail = false;
    shr16_set_ena(axis);
    startWakeTime = millis();
    MotReturn ret = MR_Success;
    if (withFindaDetection or withIR_SENSORDetection) ret = MR_Failed;
    float vMax = speed;
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

    while (stepsLeft) {
        switch (axis) {
        case AX_PUL:
            PIN_STP_PUL_HIGH;
            PIN_STP_PUL_LOW;
            if (withStallDetection && !digitalRead(A3)) { // stall detected
                delay(50); // delay to release the stall detection
                return MR_Failed;
            }
            if (withFindaDetection && ( steps > 0 ) && isFilamentLoaded()) {
              return MR_Success;
            }
            if (withFindaDetection && ( steps < 0 ) && (isFilamentLoaded() == false)) {
              return MR_Success;
            }
            if (withIR_SENSORDetection && IR_SENSOR) {
                IR_SENSOR = false;
                return MR_Success;
            }
            break;
        case AX_IDL:
            PIN_STP_IDL_HIGH;
            PIN_STP_IDL_LOW;
            if (withStallDetection && !digitalRead(A5)) { // stall detected
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
                } else {
                  return MR_Failed;
                }
            }
            break;
        case AX_SEL:
            PIN_STP_SEL_HIGH;
            PIN_STP_SEL_LOW;
            if (withStallDetection && !digitalRead(A4)) { // stall detected
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
