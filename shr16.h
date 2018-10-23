// shr16.h - 16bit shift register (2x74595)

/**
  * shr16 is a C module for setting bits of the 16-bit
  * shift register.
  *
  * According to hardware revistion 0.3 following pins are used:
  * D9: Data
  * D10: Latch
  * D13: Clock
  *
  * The 16 bits are shifted out with MSB first.
  *
  */


#ifndef _SHR16_H
#define _SHR16_H

#include <inttypes.h>
#include "config.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

extern uint16_t shr16_v;

void shr16_init(void);
void shr16_write(uint16_t v);
void shr16_set_led(uint16_t led);
void shr16_set_ena(uint8_t ena);
void shr16_set_dir(uint8_t dir);
uint8_t shr16_get_ena(void);
uint8_t shr16_get_dir(void);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)
#endif //_SHR16_H
