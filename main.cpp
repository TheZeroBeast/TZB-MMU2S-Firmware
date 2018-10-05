//! @file

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
#include <avr/wdt.h>
#include "permanent_storage.h"


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

//! @brief Initialization after reset
//!
//! button | action
//! ------ | ------
//! middle | enter setup
//! right  | continue after error
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 00 | 00 | 00 | 00 | 0b | Shift register initialized
//! 00 | 00 | 00 | 0b | 00 | uart initialized
//! 00 | 00 | 0b | 00 | 00 | spi initialized
//! 00 | 0b | 00 | 00 | 00 | tmc2130 initialized
//! 0b | 00 | 00 | 00 | 00 | A/D converter initialized
//! b0 | b0 | b0 | b0 | b0 | Error, filament detected, still present
//! 0b | 0b | 0b | 0b | 0b | Error, filament detected, no longer present, continue by right button click
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
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


	// if FINDA is sensing filament do not home
	while (digitalRead(A1) == 1)
	{
		while (Btn::right != buttonClicked())
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
		}
	}
	
	home();
	tmc2130_init(0); // trinamic
	
	// check if to goto the settings menu
	if (buttonClicked() == Btn::middle)
	{
		setupMenu();
	}
	
	
}



//! @brief Select filament menu
//!
//! Select filament by pushing left and right button, park position can be also selected.
//!
//! button | action
//! ------ | ------
//! left   | select previous filament
//! right  | select next filament
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 01 | 00 | 00 | 00 | 00 | filament 1
//! 00 | 01 | 00 | 00 | 00 | filament 2
//! 00 | 00 | 01 | 00 | 00 | filament 3
//! 00 | 00 | 00 | 01 | 00 | filament 4
//! 00 | 00 | 00 | 00 | 01 | filament 5
//! 00 | 00 | 00 | 00 | bb | park position
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
void manual_extruder_selector()
{
	shr16_set_led(1 << 2 * (4 - active_extruder));

	if ((Btn::left|Btn::right) & buttonClicked())
	{
		delay(500);

		switch (buttonClicked())
		{
		case Btn::right:
			if (active_extruder < 5)
			{
				select_extruder(active_extruder + 1);
			}
			break;
		case Btn::left:
			if (active_extruder > 0) select_extruder(active_extruder - 1);
			break;

		default:
			break;
		}
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

//! @brief main loop
//!
//! It is possible to manually select filament and feed it when not printing.
//!
//! button | action
//! ------ | ------
//! middle | feed filament
//!
//! @copydoc manual_extruder_selector()
void loop()
{
	process_commands(uart_com);

	if (!isPrinting)
	{
		manual_extruder_selector();
		if(Btn::middle == buttonClicked() && active_extruder < 5)
		{
			shr16_set_led(2 << 2 * (4 - active_extruder));
			delay(500);
			if (Btn::middle == buttonClicked())
			{
				feed_filament();
			}
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



} // extern "C"
