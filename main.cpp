//main.cpp

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <avr/io.h>
#include "shr16.h"
#include "tmc2130.h"
#include "uart.h"


void setup()
{
	shr16_init(); // shift register
	shr16_set_led(0x3ff); // set all leds on

	tmc2130_init(); // trinamic

	uart0_init(); //uart0 - usb
	stdout = uart0out; //stdout = uart0

	uart1_init(); //uart1
//	stdout = uart1out; //stdout = uart1

	printf_P(PSTR("start\n"));
}

uint16_t _leds = 1;

void loop()
{
	printf_P(PSTR("loop\n"));
	delay(500);
	shr16_set_led(_leds);
	_leds <<= 1;
	_leds &= 0x03ff;
	if (_leds == 0) _leds = 1;
}
