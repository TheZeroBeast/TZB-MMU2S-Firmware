//tmc2130.h - Trinamic stepper driver
#ifndef _TMC2130_H
#define _TMC2130_H

#include <inttypes.h>
#include "config.h"

#define TMC2130_SG_THR         4       // SG_THR default
#define TMC2130_TCOOLTHRS      450     // TCOOLTHRS default


#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


extern int8_t tmc2130_init();

extern void tmc2130_do_step(uint8_t axis_mask);

extern uint16_t tmc2130_read_sg(uint8_t axis);


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
#endif //_TMC2130_H
