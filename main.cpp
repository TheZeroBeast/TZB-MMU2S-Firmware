//main.cpp

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "adc.h"
#include "uart.h"
#include "spi.h"
#include "tmc2130.h"
#include "abtn3.h"
#include "mmctl.h"
#include "motion.h"
#include "Buttons.h"
//#include "EEPROM.h"
#include <avr/wdt.h>


int8_t sys_state = 0;
uint8_t sys_signals = 0;
int _loop = 0;
int _c = 0;

#if (UART_COM == 0)
FILE* uart_com = uart0io;
#elif (UART_COM == 1)
FILE* uart_com = uart1io;
#endif //(UART_COM == 0)

extern "C" {
void process_commands(FILE* inout);
}

//initialization after reset
void setup()
{

	shr16_init(); // shift register
	led_blink(0);

	uart0_init(); //uart0
	uart1_init(); //uart1
	led_blink(1);

#if (UART_STD == 0)
	stdin = uart0io; // stdin = uart0
	stdout = uart0io; // stdout = uart0
#elif (UART_STD == 1)
	stdin = uart1io; // stdin = uart1
	stdout = uart1io; // stdout = uart1
#endif //(UART_STD == 1)

	fprintf_P(uart_com, PSTR("start\n")); //startup message

	spi_init();
	led_blink(2);

	tmc2130_init(1); // trinamic
	led_blink(3);

	adc_init(); // ADC
	led_blink(4);

	shr16_set_ena(7);
	shr16_set_led(0x000);

	init_Pulley();


	// if FINDA is sensing filament do not home and try to unload 
	if (digitalRead(A1) == 1)
	{
		do
		{
			if (digitalRead(A1) == 1)
			{
				shr16_set_led(0x2aa);
			}
			else
			{
				shr16_set_led(0x155);
			}
			delay(300);
			shr16_set_led(0x000);
			delay(300);
		} while (buttonClicked() == 0);
	}
	
	home();
	tmc2130_init(0); // trinamic



	
	// read correction to bowden tube
	if (eeprom_read_byte((uint8_t*)0) != 0 && eeprom_read_byte((uint8_t*)0) < 200)
	{
		lengthCorrection = eeprom_read_byte((uint8_t*)0);
	}
	else
	{
		lengthCorrection = 100;
	}
	
	// check if to goto the settings menu
	if (buttonClicked() == 2)
	{
		setupMenu();
	}
	
	
}

//main loop
void loop()
{
	
	process_commands(uart_com);

	if (!isPrinting)
	{
		
		if (buttonClicked() != 0)
		{ 
			delay(500); 

			switch (buttonClicked())
			{
				case 1:
					if (active_extruder < 5)
					{
							select_extruder(active_extruder + 1);
					}
					break;
				case 2:
					if (active_extruder < 5)
					{
						shr16_set_led(2 << 2 * (4 - active_extruder));
						delay(500);
						if (buttonClicked() == 2)
						{
							feed_filament();
						}
					}
					break;
				case 4:
					if (active_extruder > 0) select_extruder(active_extruder - 1);
					break;

				default:
					break;
			}
			shr16_set_led(1 << 2 * (4 - active_extruder));
			delay(500);
		}

		if (active_extruder == 5)
		{
			shr16_set_led(2 << 2 * 0);
			delay(50);
			shr16_set_led(1 << 2 * 0);
			delay(50);
		}
	}

	 
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
	int value0 = 0;

	if ((count > 0) && (c == 0))
	{
		//line received
		//printf_P(PSTR("line received: '%s' %d\n"), line, count);
		count = 0;
		if (sscanf_P(line, PSTR("T%d"), &value) > 0)
		{
			//T-code scanned
			if ((value >= 0) && (value < EXTRUDERS))
			{
				switch_extruder_withSensor(value);

				delay(200);
				fprintf_P(inout, PSTR("ok\n"));
			}
		}
		else if (sscanf_P(line, PSTR("L%d"), &value) > 0)
		{
			// Load filament
			if ((value >= 0) && (value < EXTRUDERS) && !isFilamentLoaded)
			{

				select_extruder(value);
				feed_filament();

				delay(200);
				fprintf_P(inout, PSTR("ok\n"));

			}
		}
		else if (sscanf_P(line, PSTR("U%d"), &value) > 0)
		{
			// Unload filament
			unload_filament_withSensor();
			delay(200);
			fprintf_P(inout, PSTR("ok\n"));

			isPrinting = false;
			select_extruder(0);
		}
		else if (sscanf_P(line, PSTR("X%d"), &value) > 0)
		{
			if (value == 0) // MMU reset
				wdt_enable(WDTO_15MS);
		}
		else if (sscanf_P(line, PSTR("P%d"), &value) > 0)
		{
			if (value == 0) // Read finda
				fprintf_P(inout, PSTR("%dok\n"), digitalRead(A1));
		}
		else if (sscanf_P(line, PSTR("S%d"), &value) > 0)
		{
			if (value == 0) // return ok
				fprintf_P(inout, PSTR("ok\n"));
			else if (value == 1) // Read version
				fprintf_P(inout, PSTR("%dok\n"), FW_VERSION);
			else if (value == 2) // Read build nr
				fprintf_P(inout, PSTR("%dok\n"), FW_BUILDNR);
		}
		else if (sscanf_P(line, PSTR("F%d %d"), &value, &value0) > 0)
		{
			if (((value >= 0) && (value < EXTRUDERS)) &&
				((value0 >= 0) && (value0 <= 2)))
			{
				filament_type[value] = value0;
				fprintf_P(inout, PSTR("ok\n"));
			}
		}
		else if (sscanf_P(line, PSTR("C%d"), &value) > 0)
		{
			if (value == 0) //C0 continue loading current filament (used after T-code), maybe add different code for each extruder (the same way as T-codes) in the future?
			{
				load_filament_inPrinter();
				fprintf_P(inout, PSTR("ok\n"));
			}
		}
		else if (sscanf_P(line, PSTR("E%d"), &value) > 0)
		{
			if ((value >= 0) && (value < EXTRUDERS)) //Ex: eject filament
			{
				eject_filament(value);
				fprintf_P(inout, PSTR("ok\n"));
			}
		}
		else if (sscanf_P(line, PSTR("R%d"), &value) > 0)
		{
			if (value == 0) //R0: recover after eject filament
			{
				recover_after_eject();
				fprintf_P(inout, PSTR("ok\n"));
			}
		}
	}
	else
	{ //nothing received
	}
}



}
