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


bool switch_extruder(int new_extruder)
{
	int previous_extruder = active_extruder;
	active_extruder = new_extruder;
	
	printf_P(PSTR("Current extruder :%d\n"), previous_extruder);
	printf_P(PSTR("New extruder :%d\n"), active_extruder);

	if (previous_extruder == active_extruder)
	{
		if (!isFilamentLoaded) load_filament(); // just load filament if not loaded
	}
	else
	{
		if (isFilamentLoaded) { unload_filament(); } // unload filament first
		set_positions(previous_extruder, active_extruder); // move idler and selector to new filament position
		load_filament(); // load new filament
	}
	
	shr16_set_led(1 << 2 * (4-active_extruder));
	return true;
}


void demo_switch()
{
	int _delay = 10000;

	switch_extruder(1);
	if (!isIdlerParked) park_idler(false); // park idler for free movement
	delay(_delay);
	shr16_set_led(0x2aa);
	delay(800);
	shr16_set_led(0x155);

	switch_extruder(3);
	if (!isIdlerParked) park_idler(false); // park idler for free movement
	delay(_delay);
	shr16_set_led(0x2aa);
	delay(800);
	shr16_set_led(0x155);

	switch_extruder(0);
	if (!isIdlerParked) park_idler(false); // park idler for free movement
	delay(_delay);
	shr16_set_led(0x2aa);
	delay(800);
	shr16_set_led(0x155);

	switch_extruder(2);
	if (!isIdlerParked) park_idler(false); // park idler for free movement
	delay(_delay);
	shr16_set_led(0x2aa);
	delay(800);
	shr16_set_led(0x155);

	switch_extruder(3);
	if (!isIdlerParked) park_idler(false); // park idler for free movement
	delay(_delay);
	shr16_set_led(0x2aa);
	delay(800);
	shr16_set_led(0x155);


}