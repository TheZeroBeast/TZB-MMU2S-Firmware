//main.cpp

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "timer0.h"
#include "shr16.h"
#include "adc.h"
#include "uart.h"
#include "spi.h"
#include "tmc2130.h"
#include "abtn3.h"
#include "mmctl.h"

#ifdef _DIAG
#include "diag.h"
#endif //_DIAG


int8_t sys_state = 0;
uint8_t sys_signals = 0;


void process_commands(FILE* inout);
void process_signals(void);
void process_buttons(void);


//initialization after reset
void setup()
{
	uint16_t sp = SP;

	asm("cli"); // disable interrupts

	timer0_init(); // system timer

	shr16_init(); // shift register
	shr16_set_led(0x3ff); // set all leds on

	uart0_init(); // uart0 - usb

#if (UART_STD == 0)
	stdin = uart0io; // stdin = uart0
	stdout = uart0io; // stdout = uart0
#endif //(UART_STD == 0)

	uart1_init(); //uart1

#if (UART_STD == 1)
	stdin = uart1io; // stdin = uart1
	stdout = uart1io; // stdout = uart1
#endif //(UART_STD == 1)

	spi_init();

	int8_t tmc = tmc2130_init(); // trinamic

	adc_init(); // ADC

	asm("sei"); // enable interrupts

	delay(50);

	printf_P(PSTR("start\n"));
	printf_P(PSTR("SP=%04x\n"), sp);
	
	if (tmc < 0)
	{
		if (-tmc & 1) printf_P(PSTR("TMC2130-0 error\n"));
		if (-tmc & 2) printf_P(PSTR("TMC2130-1 error\n"));
		if (-tmc & 4) printf_P(PSTR("TMC2130-2 error\n"));
	}
//	shr16_set_led(0x000); // set all leds off
//	shr16_set_led(0x155); // set all green leds on, red off
	shr16_set_led(0x2aa); // set all red leds on, green off

	shr16_set_ena(7);

}

//main loop
void loop()
{
	process_commands(uart0io);
//	process_commands(uart1io);

	process_signals();

// LED TEST
/*	delay(100);
	shr16_set_led(_leds);
	_leds <<= 1;
	_leds &= 0x03ff;
	if (_leds == 0) _leds = 1;*/
}

void cmd_reset(FILE* inout)
{
	fprintf_P(inout, PSTR("RESET\n"));
	fflush(inout);
	delay(100);
	setup();
//	asm("sei");
//	asm("jmp 0");
}


extern "C" {

void process_commands(FILE* inout)
{
	char line[32];
	int value = 0;
	int n = 0;
	int r = 0;
	if ((r = fscanf_P(inout, PSTR("%31[^\n]%n"), line, &n)) > 0)
	{ //line received
		bool retOK = false;
		line[n] = 0;
		if ((line[n - 1] == '\r') || (line[n - 1] == '\n'))
			line[n - 1] = 0; //trim cr/lf
		if (line[0] == 0) //empty line
		{}
		else if (strcmp_P(line, PSTR("RESET")) == 0)
			cmd_reset(inout);
#ifdef _DIAG
		else if (strcmp_P(line, PSTR("UART_BRIDGE")) == 0)
			cmd_uart_bridge(inout);
		else if (strcmp_P(line, PSTR("DIAG_UART1")) == 0)
			cmd_diag_uart1(inout);
		else if (sscanf_P(line, PSTR("DIAG_TMC%d"), &value) > 0)
			cmd_diag_tmc(inout, (uint8_t)value);
#endif //_DIAG
		else if (strcmp_P(line, PSTR("HOME0")) == 0)
			retOK = home_idler();
		else if (strcmp_P(line, PSTR("HOME1")) == 0)
			retOK = home_selector();
		else if (strcmp_P(line, PSTR("MOVE1")) == 0)
			retOK = move_selector();
		else if (strcmp_P(line, PSTR("TEST")) == 0)
		{
			shr16_set_dir(7);
			for (int i = 0; i < 1000; i++)
			{
				tmc2130_do_step(7);
				delay(1);
			}
		}
		else if (strcmp_P(line, PSTR("TEST1")) == 0)
		{
			shr16_set_dir(0);
			for (int i = 0; i < 1000; i++)
			{
				tmc2130_do_step(7);
				delay(1);
			}
		}
		else if (sscanf_P(line, PSTR("T%d"), &value) > 0)
		{ //T-code scanned
			if ((value >= 0) && (value < EXTRUDERS))
			{
				switch_extruder(value);
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
//		printf_P(PSTR("line: [0]=%d [1]=%d [n-1]=%d [n]=%d\n"), line[0], line[1], line[n-1], line[n]);
		if (retOK) printf_P(PSTR("OK\n"));
	}
	else
	{ //nothing received
	}
}

void process_signals(void)
{
	// test signal 0
	if (SIG_GET(0))
	{
		SIG_CLR(0);
		printf_P(PSTR("SIG0\n"));
	}
	// buttons changed
	if (SIG_GET(SIG_ID_BTN))
	{
		SIG_CLR(SIG_ID_BTN);
		process_buttons();
	}
}

void process_buttons(void)
{
	if (abtn3_clicked(0)) 
	{
//		printf_P(PSTR("BTN0\n"));
		if (active_extruder > 0) switch_extruder(active_extruder - 1);
	}
	if (abtn3_clicked(1))
	{
//		printf_P(PSTR("BTN1\n"));
		for (uint8_t i = 0; i < 5; i++)
		{
			shr16_set_led(1 << (2 * i));
			delay(100);
		}
		switch_extruder(2);
	}
	if (abtn3_clicked(2))
	{
//		printf_P(PSTR("BTN2\n"));
		if ((active_extruder >= 0) && (active_extruder < (EXTRUDERS - 1))) switch_extruder(active_extruder + 1);
	}
}


void _every_1ms(void)
{
}

void _every_10ms(void)
{
	adc_cyc();
}

//int adc = 0;

void _every_100ms(void)
{
//	printf_P(PSTR("_every_100ms %d\n"), adc);
}

void _adc_ready(void)
{
//	printf_P(PSTR("adc_ready %d\n"), adc_val[0]);
//	adc = adc_val[0];
	if (abtn3_update()) //update buttons
		SIG_SET(SIG_ID_BTN); //set signal
}

}