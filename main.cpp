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
#include "EEPROM.h"
#include <avr/wdt.h>


int8_t sys_state = 0;
uint8_t sys_signals = 0;
int _loop = 0;
int _c = 0;
 
extern "C" {
void process_commands(FILE* inout);


//int buttonClicked();
}

//initialization after reset
void setup()
{

	shr16_init(); // shift register
	led_blink(0);

	uart1_init(); //uart1
	led_blink(1);

#if (UART_STD == 1)
	stdin = uart1io; // stdin = uart1
	stdout = uart1io; // stdout = uart1
#endif //(UART_STD == 1)

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
	
	process_commands(uart1io);

	if (!isPrinting)
	{
		
		if (buttonClicked() != 0)
		{ 
			delay(500); 

			switch (buttonClicked())
			{
				case 1:
					if (active_extruder < 4) select_extruder(active_extruder + 1);
					break;
				case 2:
					shr16_set_led(2 << 2 * (4 - active_extruder));
					delay(1000);
					if (buttonClicked() == 2)
					{
						feed_filament();
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

	if ((count > 0) && (c == 0))
	{
		//line received
		//printf_P(PSTR("line received: '%s' %d\n"), line, count);
		count = 0;
		bool retOK = false;


		if (sscanf_P(line, PSTR("T%d"), &value) > 0)
		{
			//T-code scanned
			if ((value >= 0) && (value < EXTRUDERS))
			{
				retOK = switch_extruder_withSensor(value);

				delay(200);
				if (retOK)
				{
					fprintf_P(inout, PSTR("ok\n"));
					load_filament_inPrinter();
				}
				else
				{
					fprintf_P(inout, PSTR("ok\n"));
				}

			}
		}
		 
		if (sscanf_P(line, PSTR("L%d"), &value) > 0)
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
		
		if (sscanf_P(line, PSTR("U%d"), &value) > 0)
		{
			// Unload filament
			unload_filament_withSensor();
			delay(200);
			fprintf_P(inout, PSTR("ok\n"));

			isPrinting = false;
			select_extruder(0);
		}

		if (sscanf_P(line, PSTR("X%d"), &value) > 0)
		{
			// MMU reset
               if(value==0)
                    {
                    wdt_enable(WDTO_15MS);
                    }
		}


	}
	else
	{ //nothing received
	}
}



}
