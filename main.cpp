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

	shr16_set_ena(7);
}

//uint16_t _leds = 1;

bool home_idler()
{
	shr16_set_dir(shr16_get_dir() & ~1);
	for (int i = 0; i < 3000; i++)
	{
		tmc2130_do_step(1);
		delay(1);
	}
	return true;
}


bool home_selector()
{
//	shr16_set_dir(shr16_get_dir() & ~2);
	shr16_set_dir(shr16_get_dir() | 2);
	int i = 0; for (; i < 4000; i++)
	{
		tmc2130_do_step(2);
		delay(1);
		uint16_t sg = tmc2130_read_sg(1);
		if ((i > 16) && (sg < 100))
			break;
		printf_P(PSTR("SG=%d\n"), tmc2130_read_sg(1));
	}
	return (i < 4000);
}

bool move_selector()
{
	shr16_set_dir(shr16_get_dir() & ~2);
//	shr16_set_dir(shr16_get_dir() | 2);
	for (int i = 0; i < 2000; i++)
	{
		tmc2130_do_step(2);
		delay(1);
	}
	return true;
}


bool home_()
{
	shr16_set_dir(shr16_get_dir() & ~2);
	int i = 0; for (; i < 3000; i++)
	{
		tmc2130_do_step(2);
		delay(1);
	}
	return true;
}

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
			// TODO - reset
		}
		else if (strcmp_P(line, PSTR("HOME0")) == 0)
		{
			home_idler();
			printf_P(PSTR("HOME 0\n"));
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

bool switch_extruder(int new_extruder)
{
	//TODO - control motors
	active_extruder = new_extruder;
	shr16_set_led(1 << 2*active_extruder);
	return true;
}

uint8_t buttons_sample(void)
{
	int raw = analogRead(BTN_APIN);
	// Button 1 - 0
	// Button 2 - 344
	// Button 3 - 516
	if (raw < 10) return 1;
	else if (raw > 300 && raw < 400) return 2;
	else if (raw > 450 && raw < 600) return 4;
	return(0);
}
