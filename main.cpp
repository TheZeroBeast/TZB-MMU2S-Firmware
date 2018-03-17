//main.cpp

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "uart.h"
#include "spi.h"
#include "tmc2130.h"
#include "abtn3.h"
#include "mmctl.h"


int active_extruder = -1;


void setup()
{
	shr16_init(); // shift register
	shr16_set_led(0x3ff); // set all leds on

	uart0_init(); //uart0 - usb
	stdin = uart0io; //stdin = uart0
	stdout = uart0io; //stdout = uart0

	uart1_init(); //uart1
//	stdin = uart1io; //stdin = uart1
//	stdout = uart1io; //stdout = uart1

	printf_P(PSTR("start\n"));

#ifdef TMC2130_SPI_ARDUINO
	SPI.begin();
#else //TMC2130_SPI_ARDUINO
	spi_init();
#endif //TMC2130_SPI_ARDUINO

	tmc2130_init(); // trinamic

//	shr16_set_led(0x000); // set all leds off
//	shr16_set_led(0x155); // set all green leds on, red off
	shr16_set_led(0x2aa); // set all red leds on, green off

	shr16_set_ena(7);
}

//uint16_t _leds = 1;

void loop()
{
	char line[32];
	int value = 0;
	int n = 0;
	int r = 0;
//	static char c = 0;

//	putc(c, uart1io);
//	c++;
//	while ((r = fscanf_P(uart0io, PSTR("%c"), &c)) > 0)
//		putc(c, uart1io);
//	while ((r = fscanf_P(uart1io, PSTR("%c"), &c)) > 0)
//		putc(c, uart0io);

//	while ((r = fscanf_P(uart0io, PSTR("%c"), &c)) > 0)
//		putc(c, uart0io);

/*
		if ((r = fscanf_P(uart0io, PSTR("%s%n"), line, &n)) > 0)
		{
			line[n] = 0;
			printf_P(PSTR("scan: %d %d\n"), r, n);
			fprintf_P(uart0io, PSTR("%s"), line);
		}*/


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
//			asm("jmp 0x00000");
			setup();
			// TODO - reset
		}
		else if (strcmp_P(line, PSTR("HOME0")) == 0)
		{
			printf_P(PSTR("HOME 0 %d\n"), home_idler()?1:0);
		}
		else if (strcmp_P(line, PSTR("HOME1")) == 0)
		{
			printf_P(PSTR("HOME 1 %d\n"), home_selector()?1:0);
		}
		else if (strcmp_P(line, PSTR("MOVE1")) == 0)
		{
			move_selector();
			printf_P(PSTR("MOVE 1\n"));
		}
		else if (strcmp_P(line, PSTR("TEST")) == 0)
		{
			shr16_set_dir(7);
			for (int i = 0; i < 1000; i++)
			{
				tmc2130_do_step(7);
				delay(1);
			}
			printf_P(PSTR("TEST OK\n"));
			// TODO - reset
		}
		else if (strcmp_P(line, PSTR("TEST1")) == 0)
		{
			shr16_set_dir(0);
			for (int i = 0; i < 1000; i++)
			{
				tmc2130_do_step(7);
				delay(1);
			}
			printf_P(PSTR("TEST1 OK\n"));
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
		{ //unknown command
			printf_P(PSTR("Unknown command: \"%s\"\n"), line);
		}
//		printf_P(PSTR("line: '%s'\n"), line);
//		printf_P(PSTR("scan: %d %d\n"), r, n);
	}
	else
	{ //nothing received
	}
/*	uint8_t btn = button();
	switch (btn)
	{
	case 1:
		printf_P(PSTR("BUTTON 1\n"));
		if (active_extruder < (EXTRUDERS - 1))
		{
			switch_extruder(active_extruder + 1);
			printf_P(PSTR("EXTRUDER+ %d\n"), active_extruder);
		}
		break;
	case 2:
		printf_P(PSTR("BUTTON 2\n"));
		break;
	case 3:
		printf_P(PSTR("BUTTON 3\n"));
		if (active_extruder > 0)
		{
			switch_extruder(active_extruder - 1);
			printf_P(PSTR("EXTRUDER- %d\n"), active_extruder);
		}
		break;
	}*/
// LED TEST
/*	delay(100);
	shr16_set_led(_leds);
	_leds <<= 1;
	_leds &= 0x03ff;
	if (_leds == 0) _leds = 1;*/
}

