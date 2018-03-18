//timer0.c - timer0 system timer

#include "timer0.h"
#include <avr/io.h>
#include <avr/interrupt.h>


uint8_t timer0_10ms_cnt;
uint8_t timer0_100ms_cnt;
uint8_t timer0_1000ms_cnt;
uint16_t timer0_sec;


void timer0_init(void)
{
	timer0_10ms_cnt = 0;
	timer0_100ms_cnt = 0;
	timer0_1000ms_cnt = 0;
	timer0_sec = 0;
	OCR0B = 128;
	TIMSK0 |= (1<<OCIE0B);  
}

#ifdef TIMER0_EVERY_10ms
extern void TIMER0_EVERY_10ms(void);
#endif //TIMER0_EVERY_10ms

#ifdef TIMER0_EVERY_100ms
extern void TIMER0_EVERY_100ms(void);
#endif //TIMER0_EVERY_100ms

#ifdef TIMER0_EVERY_1000ms
extern void TIMER0_EVERY_1000ms(void);
#endif //TIMER0_EVERY_1000ms

extern uint8_t sys_signals;

#define SIG_SET(id) (sys_signals |= (1 << id))

ISR(TIMER0_COMPB_vect)
{
	if ((timer0_10ms_cnt--) == 0)
	{
		timer0_10ms_cnt = 10;
		if ((timer0_100ms_cnt--) == 0)
		{
			timer0_100ms_cnt = 10;
			if ((timer0_1000ms_cnt--) == 0)
			{
				timer0_1000ms_cnt = 10;
				timer0_sec++;
			}
#ifdef TIMER0_EVERY_1000ms
			else if (timer0_1000ms_cnt == 5) TIMER0_EVERY_1000ms();
#endif //TIMER0_EVERY_1000ms
		}
#ifdef TIMER0_EVERY_100ms
		else if (timer0_100ms_cnt == 5) TIMER0_EVERY_100ms();
#endif //TIMER0_EVERY_100ms
	}
#ifdef TIMER0_EVERY_10ms
	else if (timer0_10ms_cnt == 5) TIMER0_EVERY_10ms();
#endif //TIMER0_EVERY_10ms
}