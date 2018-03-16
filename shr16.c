//shr16.c - 16bit shift register (2x74595)

#include "shr16.h"
#include <avr/io.h>


uint16_t shr16_v;

void shr16_init(void)
{
	DDRC |= 0x80;
	DDRB |= 0x40;
	DDRB |= 0x20;
	PORTC &= ~0x80;
	PORTB &= ~0x40;
	PORTB &= ~0x20;
	shr16_v = 0;
	shr16_write(shr16_v);
	shr16_write(shr16_v);
}

void shr16_write(uint16_t v)
{
	PORTB &= ~0x40;
	asm("nop");
	for (uint16_t m = 0x8000; m; m >>= 1)
	{
		if (m & v)
			PORTB |= 0x20;
		else
			PORTB &= ~0x20;
		PORTC |= 0x80;
		asm("nop");
		PORTC &= ~0x80;
		asm("nop");
	}
	PORTB |= 0x40;
	asm("nop");
	shr16_v = v;
}

void shr16_set_led(uint16_t led)
{
	led = ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2);
	shr16_write((shr16_v & ~SHR16_LED_MSK) | led);
}

void shr16_set_ena(uint8_t ena)
{
}

void shr16_set_dir(uint8_t dir)
{
}
