//main.cpp

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "timer0.h"
#include "shr16.h"
#include "uart.h"
#include "spi.h"
#include "tmc2130.h"
#include "abtn3.h"
#include "mmctl.h"



//#define _STATE_BOOT  0
//#define _STATE_ERR1  1 //TMC2130 spi error - not responding 
//#define _STATE_ERR2  2 //TMC2130 motor error - short circuit
//#define _STATE_ERR3  3 //TMC2130 motor error - open circuit

//int counter = 0;

int8_t sys_state = 0;
uint8_t sys_signals = 0;

#define SIG_GET(id) (sys_signals & (1 << id))
#define SIG_SET(id) (sys_signals |= (1 << id))
#define SIG_CLR(id) asm("cli"); sys_signals &= ~(1 << id); asm("sei")

#define SIGNAL_BTN 1

void setup()
{
	timer0_init(); // system timer

	shr16_init(); // shift register
	shr16_set_led(0x3ff); // set all leds on

	uart0_init(); //uart0 - usb

#if (UART_STD == 0)
	stdin = uart0io; //stdin = uart0
	stdout = uart0io; //stdout = uart0
#endif //(UART_STD == 0)

	uart1_init(); //uart1

#if (UART_STD == 1)
	stdin = uart1io; //stdin = uart1
	stdout = uart1io; //stdout = uart1
#endif //(UART_STD == 1)

	printf_P(PSTR("start\n"));

	spi_init();

	tmc2130_init(); // trinamic

	delay(100);

//	shr16_set_led(0x000); // set all leds off
//	shr16_set_led(0x155); // set all green leds on, red off
	shr16_set_led(0x2aa); // set all red leds on, green off

	shr16_set_ena(7);

}
/*
//uint16_t _leds = 1;
#define TIMEOUT_IDS 4
#define TIMEOUT_TIM (uint16_t)(millis() >> 10)
uint8_t timeout_flg = 0;
uint8_t timeout_ovf = 0;
uint16_t timeout_exp[TIMEOUT_IDS];

void timeout_set(uint8_t id, uint16_t val)
{
	uint8_t register msk = (1 << id);
	uint16_t tim = TIMEOUT_TIM;
	val += tim;
	timeout_exp[TIMEOUT_IDS] = val;
	timeout_flg |= msk;
	timeout_ovf |= (tim > exp)?msk:0;
}

uint8_t timeout_exp(uint8_t id)
{
	uint8_t register msk = (1 << id);
	if ((timeout_flg & msk) == 0) return 1;
	uint16_t exp = timeout_exp[TIMEOUT_IDS];
	uint16_t tim = TIMEOUT_TIM;
	if (timeout_ovf & msk)?(tim < exp):(tim > exp)) timeout_flg ^= msk;
	return (timeout_flags & msk)?0:1;
}
*/
void loop()
{
	char line[32];
	int value = 0;
	int n = 0;
	int r = 0;
//	char c = 0;
//	static char c1 = 0;

//	putc(c1, uart1io);
//	c1++;
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

	if (SIG_GET(SIGNAL_BTN))
	{
		SIG_CLR(SIGNAL_BTN);
		if (abtn3_clicked(0)) 
		{
			printf_P(PSTR("BTN0\n"));
			if (active_extruder > 0) switch_extruder(active_extruder - 1);
		}
		if (abtn3_clicked(1))
		{
			printf_P(PSTR("BTN1\n"));
			for (uint8_t i = 0; i < 5; i++)
			{
				shr16_set_led(1 << (2 * i));
				delay(100);
			}
			switch_extruder(2);
		}
		if (abtn3_clicked(2))
		{
			printf_P(PSTR("BTN2\n"));
			if ((active_extruder >= 0) && (active_extruder < (EXTRUDERS - 1))) switch_extruder(active_extruder + 1);
		}
	}
//	delay(10);

	if (SIG_GET(0))
	{
		SIG_CLR(0);
		printf_P(PSTR("SIG0\n"));
	}

//	printf_P(PSTR("COUNTER=%d\n"), timer0_sec);

// LED TEST
/*	delay(100);
	shr16_set_led(_leds);
	_leds <<= 1;
	_leds &= 0x03ff;
	if (_leds == 0) _leds = 1;*/
}

extern "C" {

void _every_10ms(void)
{
	if (abtn3_update()) //update buttons
		SIG_SET(SIGNAL_BTN); //set signal
}

void _every_100ms(void)
{
}

}