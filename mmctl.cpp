//mmctl.cpp - multimaterial switcher control
#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "spi.h"
#include "tmc2130.h"
#include "mmctl.h"
#include "motion.h"
#include "Buttons.h"

int lengthCorrection = 0;
int active_extruder = -1;
int previous_extruder = -1;
bool isFilamentLoaded = false;
bool isIdlerParked = false;
int toolChanges = 0;

bool isPrinting = false;
bool isHomed = false;

bool feed_filament()
{
	bool _feed = true;
	bool _loaded = false;

	int _c = 0;
	int _delay = 0;
	park_idler(true);

	set_pulley_dir_push();
	tmc2130_init_axis_current(0, 1, 15);

	do
	{
		do_pulley_step();
		
		_c++;
		if (_c > 50) { shr16_set_led(2 << 2 * (4 - active_extruder)); };
		if (_c > 100) { shr16_set_led(0x000); _c = 0; _delay++; };

		if (digitalRead(A1) == 1) { _loaded = true; _feed = false; };
		if (buttonClicked() != 0 && _delay > 10) { _loaded = false; _feed = false; }
		delayMicroseconds(4000);
	} while (_feed);

	if (_loaded)
	{
		// unload to PTFE tube
		set_pulley_dir_pull();
		for (int i = 600; i > 0; i--)   // 570
		{
			do_pulley_step();
			delayMicroseconds(3000);
		}
	}



	tmc2130_init_axis_current(0, 0, 0);
	park_idler(false);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return true;
}

bool switch_extruder_withSensor(int new_extruder)
{
	
	isPrinting = true;
	bool _return = false;
	if (!isHomed) { home(); }
	
	if (active_extruder == 5)
	{
		move(0, -700, 0);
		active_extruder = 4;
	}
	
	
	toolChanges++;

	shr16_set_led(2 << 2 * (4 - active_extruder));

	previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder == active_extruder)
	{
		if (!isFilamentLoaded)
		{
			shr16_set_led(2 << 2 * (4 - active_extruder));
			load_filament_withSensor(); // just load filament if not loaded
			_return = true;
		}
		else
		{
			_return = false;  // nothing really happened
		}
	}
	else
	{
		if (isFilamentLoaded) { unload_filament_withSensor(); } // unload filament first
		set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
		
		shr16_set_led(2 << 2 * (4 - active_extruder));
		load_filament_withSensor(); // load new filament
		_return = true;
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return _return;


}

bool select_extruder(int new_extruder)
{

	bool _return = false;
	if (!isHomed) { home(); }

	shr16_set_led(2 << 2 * (4 - active_extruder));

	int previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder == active_extruder)
	{
		if (!isFilamentLoaded)
		{
			_return = true;
		}
	}
	else
	{
		if (new_extruder == 5)
		{
			move(0, 700, 0);
		}
		else
		{
			if (previous_extruder == 5)
			{
				move(0, -700, 0);
			}
			else
			{
				if (isIdlerParked) park_idler(true);
				set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
				park_idler(false);
			}
		}
		_return = true;
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return _return;
}

bool service_position(bool service)
{
	move(0, 600, 0);

	return true;
}

void led_blink(int _no)
{
	shr16_set_led(1 << 2 * _no);
	delay(40);
	shr16_set_led(0x000);
	delay(20);
	shr16_set_led(1 << 2 * _no);
	delay(40);

	shr16_set_led(0x000);
	delay(10);
}


