//tmc2130.h
#ifndef _TMC2130_H
#define _TMC2130_H

#include <inttypes.h>
#include "config.h"


extern int8_t tmc2130_init();

extern void tmc2130_do_step(uint8_t axis);


#endif //_TMC2130_H
