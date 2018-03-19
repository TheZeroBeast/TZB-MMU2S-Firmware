//adc.c

#include "adc.h"
#include <avr/io.h>


uint8_t adc_sta;
uint8_t adc_cnt;
uint16_t adc_val[ADC_CHAN_CNT];
uint16_t adc_sim_msk;


#ifdef ADC_READY
	extern void ADC_READY(void);
#endif //ADC_READY


void adc_init(void)
{
	adc_sim_msk = 0x00;
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	ADMUX |= (1 << REFS0);
	ADCSRA |= (1 << ADEN);
//	ADCSRA |= (1 << ADIF) | (1 << ADSC);
	DIDR0 = (ADC_CHAN_MSK & 0xff);
	DIDR2 = (ADC_CHAN_MSK >> 8);
	adc_res();
}

void adc_res(void)
{
	adc_sta = 0;
	adc_cnt = 0;
	uint8_t i; for (i = 0; i < ADC_CHAN_CNT; i++)
	if ((adc_sim_msk & (1 << i)) == 0)
		adc_val[i] = 0;
}

void adc_mux(uint8_t ch)
{
	ch &= 0x0f;
	if (ch & 0x08) ADCSRB |= (1 << MUX5);
	else ADCSRB &= ~(1 << MUX5);
	ADMUX = (ADMUX & ~(0x07)) | (ch & 0x07);
}

uint8_t adc_chan(uint8_t index)
{
	uint8_t chan = 0;
	uint16_t mask = 1;
	while (mask)
	{
		if ((mask & ADC_CHAN_MSK) && (index-- == 0)) break;
		mask <<= 1;
		chan++;
	}
	return chan;
}

void adc_cyc(void)
{
	if (adc_sta & 0x80)
	{
		uint8_t index = adc_sta & 0x0f;
		if ((adc_sim_msk & (1 << index)) == 0)
			adc_val[index] += ADC;
		if (index++ >= ADC_CHAN_CNT)
		{
			index = 0;
			adc_cnt++;
			if (adc_cnt >= ADC_OVRSAMPL)
			{
#ifdef ADC_READY
				ADC_READY();
#endif //ADC_READY
				adc_res();
			}
		}
		adc_mux(adc_chan(index));
		adc_sta = index;
	}
	else
	{
		ADCSRA |= (1 << ADSC); //start conversion
		adc_sta |= 0x80;
	}
}
