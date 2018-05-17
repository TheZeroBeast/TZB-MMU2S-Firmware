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

int active_extruder = -1;
bool isFilamentLoaded = false;
bool isIdlerParked = false;

bool isPrinting = false;
bool isHomed = false;

bool switch_extruder(int new_extruder)
{
	isPrinting = true;
	bool _return = false;
	if (!isHomed) { home(); }
  
  
    shr16_set_led(2 << 2 * (4 - active_extruder));

	int previous_extruder = active_extruder;
	active_extruder = new_extruder;

	if (previous_extruder == active_extruder)
	{
		if (!isFilamentLoaded)
		{
			load_filament(); // just load filament if not loaded
			_return = true;
		}
	}
	else
	{
		if (isFilamentLoaded) { unload_filament(); } // unload filament first
		set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
		load_filament(); // load new filament
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

	Serial.println(new_extruder);
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
		if (isIdlerParked) park_idler(true);
		set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
		park_idler(false);
		_return = true;
	}

	shr16_set_led(0x000);
	shr16_set_led(1 << 2 * (4 - active_extruder));
	return _return;
}


