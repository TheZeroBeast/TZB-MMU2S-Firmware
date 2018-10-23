// adc.h
#ifndef _ADC_H
#define _ADC_H

#include <inttypes.h>
#include "config.h"

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

extern uint8_t adc_sta;
extern uint8_t adc_cou;
extern uint16_t adc_val[ADC_CHAN_CNT];
extern uint16_t adc_sim_msk;

void adc_init(void);
void adc_res(void);
void adc_mux(uint8_t ch);
uint8_t adc_chan(uint8_t index);
void adc_cyc(void);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)
#endif //_ADC_H
