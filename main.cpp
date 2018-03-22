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
#include "motion.h"

#ifdef _DIAG
#include "diag.h"
#endif //_DIAG


int8_t sys_state = 0;
uint8_t sys_signals = 0;

extern "C" {
void process_commands(FILE* inout);
void process_signals(void);
void process_buttons(void);
}

//initialization after reset
void setup()
{
	asm("cli"); // disable interrupts

	timer0_init(); // system timer

	shr16_init(); // shift register
	shr16_set_led(0x3ff); // set all leds on

	//uart0_init(); // uart0 - usb
/*
#if (UART_STD == 0)
	stdin = uart0io; // stdin = uart0
	stdout = uart0io; // stdout = uart0
#endif //(UART_STD == 0)
*/
	uart1_init(); //uart1

#if (UART_STD == 1)
	stdin = uart1io; // stdin = uart1
	stdout = uart1io; // stdout = uart1
#endif //(UART_STD == 1)

	spi_init();

	tmc2130_init(); // trinamic

	adc_init(); // ADC

	asm("sei"); // enable interrupts

	delay(50);

	printf_P(PSTR("start\n"));


//	shr16_set_led(0x000); // set all leds off
//	shr16_set_led(0x155); // set all green leds on, red off
	shr16_set_led(0x2aa); // set all red leds on, green off

	shr16_set_ena(7);

	
	
	home();
	

}

//main loop
void loop()
{
	//process_commands(uart0io);
	process_commands(uart1io);

	process_signals();

	demo_switch();
		

}

void cmd_reset(FILE* inout)
{
	fprintf_P(inout, PSTR("RESET\n"));
	fflush(inout);
	delay(100);
	asm("sei");
	asm("jmp 0");
}


extern "C" {

void process_commands(FILE* inout)
{
	static char line[32];
	static int count = 0;
	int c = -1;
	if (count < 32)
	{
		if ((c = getc(inout)) >= 0)
		{
			if (c == '\r') c = 0;
			if (c == '\n') c = 0;
			line[count++] = c;
		}
	}
	else
	{
		count = 0;
		//overflow
	}
	int value = 0;

	if ((count > 0) && (c == 0))
	{ 
		//line received
		printf_P(PSTR("line received: '%s' %d\n"), line, count);
		count = 0;
		bool retOK = false;

		if (strcmp_P(line, PSTR("RESET")) == 0)
			cmd_reset(inout);
#ifdef _DIAG
		else if (strcmp_P(line, PSTR("UART_BRIDGE")) == 0)
			cmd_uart_bridge(inout);
		else if (strcmp_P(line, PSTR("DIAG_UART1")) == 0)
			cmd_diag_uart1(inout);
		else if (sscanf_P(line, PSTR("DIAG_TMC%d"), &value) > 0)
			cmd_diag_tmc(inout, (uint8_t)value);
#endif //_DIAG
		else if (strcmp_P(line, PSTR("HOME")) == 0)
			home();
		
		else if (sscanf_P(line, PSTR("T%d"), &value) > 0)
		{ 
			//T-code scanned
			printf_P(PSTR("T-code scanned extruder=%d\n"), value);
			if ((value >= 0) && (value < EXTRUDERS))
			{
				retOK = switch_extruder(value);
			}
			else 
				//value out of range
				printf_P(PSTR("Invalid extruder %d\n"), value);
		}

		
		else
		{ //unknown command
			printf_P(PSTR("Unknown command: \"%s\"\n"), line);
		}

		if (retOK)
		{
			printf_P(PSTR("'ok' send\n"));
			fprintf_P(inout, PSTR("ok\n"));
			load_filament_intoPrinter();
			printf_P(PSTR("loaded in printer\n"));
			if (!isIdlerParked) park_idler(false); // park idler for free movement
		}
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
	}
	if (abtn3_clicked(1))
	{
	}
	if (abtn3_clicked(2))
	{
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