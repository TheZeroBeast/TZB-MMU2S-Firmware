// Buttons.h

#ifndef _BUTTONS_h
#define _BUTTONS_h

#include <stdint.h>
#include "shr16.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "permanent_storage.h"
#include "main.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

void setupMenu();
uint8_t buttonClicked();

#endif //_BUTTONS_h
