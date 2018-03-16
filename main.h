#ifndef _MAIN_H
#define _MAIN_H


#include <inttypes.h>
#include "config.h"


extern int active_extruder;

extern bool switch_extruder(int new_extruder);


extern uint8_t buttons_state;

extern uint8_t buttons_click;

extern void buttons_update(void);

extern uint8_t button_clicked(uint8_t mask);


#endif //_MAIN_H
