#ifndef _MAIN_H
#define _MAIN_H

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "uart.h"
#include "spi.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "Buttons.h"
#include "Detect12V24V.h"
#include "permanent_storage.h"
#include "config.h"
#include "uart.h"
#include <avr/wdt.h>
#include <inttypes.h>

#define isFilamentLoaded() digitalRead(A1)

void manual_extruder_selector();

// signals from interrupt to main loop
extern bool MMU2SLoading;
extern bool inErrorState;
//extern bool initialised;
void fixTheProblem(bool showPrevious = false);
void fixSelCrash(void);
void fixIdlCrash(void);

extern uint8_t tmc2130_mode;
extern long startWakeTime;

typedef enum eFault {FAULT_IDLER_INIT_0, FAULT_IDLER_INIT_1, FAULT_IDLER_INIT_2,
                     FAULT_SELECTOR_INIT_0, FAULT_SELECTOR_INIT_1, FAULT_SELECTOR_INIT_2,
                     FAULT_PULLEY_INIT_0, FAULT_PULLEY_INIT_1, FAULT_PULLEY_INIT_2,
                    } Fault;

#endif //_MAIN_H
