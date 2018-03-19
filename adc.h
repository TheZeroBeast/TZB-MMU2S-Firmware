//adc.h
#ifndef _ADC_H
#define _ADC_H

#include <inttypes.h>
#include "config.h"


#if defined(__cplusplus)
extern "C" {
#endif //defined(__cplusplus)


extern uint8_t adc_sta;
extern uint8_t adc_cou;
extern uint16_t adc_val[ADC_CHAN_CNT];
extern uint16_t adc_sim_msk;


extern void adc_init(void);

extern void adc_res(void);

extern void adc_mux(uint8_t ch);

extern uint8_t adc_chan(uint8_t index);

extern void adc_cyc(void);


#if defined(__cplusplus)
}
#endif //defined(__cplusplus)
#endif //_ADC_H
