//tmc2130.h - Trinamic stepper driver
#ifndef _TMC2130_H
#define _TMC2130_H

#include <inttypes.h>
#include "config.h"

#define TMC2130_SG_THR         4       // SG_THR default
#define TMC2130_TCOOLTHRS      450     // TCOOLTHRS default

#define TMC2130_CHECK_SPI 0x01
#define TMC2130_CHECK_MSC 0x02
#define TMC2130_CHECK_MOC 0x04
#define TMC2130_CHECK_STP 0x08
#define TMC2130_CHECK_DIR 0x10
#define TMC2130_CHECK_ENA 0x20
#define TMC2130_CHECK_OK  0x3f


#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


extern int8_t tmc2130_init();

extern int8_t tmc2130_init_axis(uint8_t axis);

extern uint8_t tmc2130_check_axis(uint8_t axis);

extern void tmc2130_do_step(uint8_t axis_mask);

extern uint16_t tmc2130_read_sg(uint8_t axis);


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
#endif //_TMC2130_H
