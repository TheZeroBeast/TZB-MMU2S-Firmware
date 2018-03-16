//main.cpp

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "tmc2130.h"
#include "uart.h"


int active_extruder = -1;


void setup()
{
	shr16_init(); // shift register
	shr16_set_led(0x3ff); // set all leds on

	tmc2130_init(); // trinamic

	uart0_init(); //uart0 - usb
	stdin = uart0io; //stdin = uart0
	stdout = uart0io; //stdout = uart0

	uart1_init(); //uart1
//	stdin = uart1io; //stdin = uart1
//	stdout = uart1io; //stdout = uart1

	printf_P(PSTR("start\n"));

//	shr16_set_led(0x000); // set all leds off
//	shr16_set_led(0x155); // set all green leds on, red off
	shr16_set_led(0x2aa); // set all red leds on, green off
}

//uint16_t _leds = 1;

void loop()
{
	char line[32];
	int value = 0;
	int n = 0;
	int r = 0;
	if ((r = fscanf_P(uart0io, PSTR("%31[^\n]%n"), line, &n)) > 0)
	{ //line received
		line[n] = 0;
		if ((r == 1) && (n == 1))
		{ //empty line
//			printf_P(PSTR("Empty line\n"));
			getc(uart0io); //read LF character
		}
		else if (strcmp_P(line, PSTR("RESET")) == 0)
		{
			printf_P(PSTR("RESET OK\n"));
			// TODO - reset
		}
		else if (sscanf_P(line, PSTR("T%d"), &value) > 0)
		{ //T-code scanned
			if ((value >= 0) && (value < EXTRUDERS))
			{
				switch_extruder(value);
				printf_P(PSTR("OK %d\n"), value);
			}
			else //value out of range
				printf_P(PSTR("Invalid extruder %d\n"), value);
		}
		else
		{
			printf_P(PSTR("Unknown command: \"%s\"\n"), line);
		}
//		printf_P(PSTR("line: '%s'\n"), line);
//		printf_P(PSTR("scan: %d %d\n"), r, n);
	}
	else
	{ //nothing received
	}

/*// LED TEST
	delay(100);
	shr16_set_led(_leds);
	_leds <<= 1;
	_leds &= 0x03ff;
	if (_leds == 0) _leds = 1;*/
}

bool switch_extruder(int new_extruder)
{
	//TODO - control motors
	active_extruder = new_extruder;
	shr16_set_led(1 << 2*active_extruder);
	return true;
}