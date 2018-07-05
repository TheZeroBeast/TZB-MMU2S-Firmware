// 
// 
// 

#include "Buttons.h"
#include "shr16.h"
#include "mmctl.h"
#include "motion.h"

const int ButtonPin = A2;

void settings_bowden_length();

void setupMenu()
{
	shr16_set_led(0x000);
	delay(200);
	shr16_set_led(0x2aa);
	delay(1200);
	shr16_set_led(0x000);
	delay(600);

	int _menu = 0;
	bool _exit = false;

		

	do
	{
		shr16_set_led(1 << 2 * 4);
		delay(1);
		shr16_set_led(2 << 2 * 4);
		delay(1);
		shr16_set_led(2 << 2 * _menu);
		delay(1);

		switch (buttonClicked())
		{
		case 1:
			if (_menu > 0) { _menu--; delay(800); }
			break;
		case 2:
				
			switch (_menu)
			{
				case 1:
					settings_bowden_length();
					break;

				case 4: // exit menu
					_exit = true;
					break;
			}
			break;
		case 4:
			if (_menu < 4) { _menu++; delay(800); }
			break;
		}
		
	} while (!_exit);


	shr16_set_led(0x000);
	delay(400);
	shr16_set_led(0x2aa);
	delay(400);
	shr16_set_led(0x000);
	delay(400);

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
}


void settings_bowden_length()
{
	// load filament above Bondtech gears to check correct length of bowden tube
	if (!isFilamentLoaded)
	{
		load_filament_withSensor();
		do
		{
			shr16_set_led(1 << 2 * 4);
			delay(50);
			shr16_set_led(2 << 2 * 4);
			delay(50);
			shr16_set_led(2 << 2 * 1);
			delay(50);
		} while (buttonClicked() != 2);
		unload_filament_withSensor();
	}
}


int buttonClicked()
{
	int raw = analogRead(ButtonPin);
	int _return = 0;

	 
	if (raw < 50) _return = 1;
	if (raw > 80 && raw < 100) _return = 2;
	if (raw > 160 && raw < 180) _return = 4;

	return(_return);
}

