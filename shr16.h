//shr16.h - 16bit shift register (2x74595)
#ifndef _SHR16_H
#define _SHR16_H

#include <inttypes.h>
#include "config.h"



#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


extern uint16_t shr16_v;

extern void shr16_init(void);

extern void shr16_write(uint16_t v);

extern void shr16_set_led(uint16_t led);

extern void shr16_set_ena(uint8_t ena);

extern void shr16_set_dir(uint8_t dir);

extern uint8_t shr16_get_ena(void);

extern uint8_t shr16_get_dir(void);


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
#endif //_SHR16_H
