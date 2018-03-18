//timer0.h - timer0 system timer
#ifndef _TIMER0_H
#define _TIMER0_H

#include <inttypes.h>
#include "config.h"


#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


extern uint16_t timer0_sec;

extern void timer0_init(void);


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
#endif //_TIMER0_H
