// abtn3.h - 3 buttons on analog channel
#ifndef _ABTN3_H
#define _ABTN3_H

#include <inttypes.h>
#include "config.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

extern uint8_t abtn_state;
extern uint8_t abtn_click;

uint8_t abtn3_update(void);
uint8_t abtn3_clicked(uint8_t btn);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)
#endif //_ABTN3_H
