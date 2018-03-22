#include "motion.h"
#include "shr16.h"
#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Arduino.h>
#include "main.h"
#include "mmctl.h"


const int selector_steps_after_homing = -3700;
const int idler_steps_after_homing = -15;

const int selector_steps = 2700/4;
const int idler_steps = 180 / 4;
const int idler_parking_steps = (idler_steps / 2)+10;

const int bowden_length = 1000;


void move(int _idler, int _selector);

int set_idler_direction(int _steps);
int set_selector_direction(int _steps);

void cut_filament();

void park_idler(bool _unpark);
void load_filament();
void load_filament_intoPrinter();
void unload_filament();
void set_positions(int _current_extruder, int _next_extruder);



void cut_filament()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	shr16_set_dir(shr16_get_dir() & ~4);

	move(0,800);
	delay(2000);

	int _speed = 4000;
	for (int i = 0; i < 400; i++)
	{
		PORTD |= 0x40;;
		asm("nop");
		PORTD &= ~0x40;
		asm("nop");
		delayMicroseconds(_speed);
	}
	
	
	int _selector = set_selector_direction(-800);
	do
	{
			shr16_set_dir(shr16_get_dir() & ~2);
			PORTD |= 0x10; _selector--;
			asm("nop");
			PORTD &= ~0x10;
			PORTB &= ~0x10;
			asm("nop");
			delayMicroseconds(15000);
		
	} while (_selector != 0);
	


}



void set_positions(int _current_extruder, int _next_extruder)
{
	// steps to move to new position
	int _selector_steps = ((_current_extruder - _next_extruder) * selector_steps) * -1;
	int _idler_steps = (_current_extruder - _next_extruder) * idler_steps;

	// move both to new position
	move(_idler_steps, _selector_steps);
}

void load_filament()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	// 4150 = 100 mm

	shr16_set_dir(shr16_get_dir() & ~4);
	int _speed = 4000;
	
	for (int i = 0; i < 6500; i++)  // 16400   --> shorter only for demo
	{
	PORTD |= 0x40;;
	asm("nop");
	PORTD &= ~0x40;
	asm("nop");


	if (i > 100 && i < 800 && _speed > 1500) _speed = _speed - 2;
	if (i > 800 && i < 2000 && _speed > 600) _speed = _speed - 3;
	if (i > 2000 && i < 5500 && _speed > 260) _speed = _speed - 1;
	if (i > 5000 && _speed < 1000) _speed = _speed + 1;
	delayMicroseconds(_speed);
	}
	
	isFilamentLoaded = true;  // filament loaded 
}

void load_filament_intoPrinter()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
										 // 4150 = 100 mm

	shr16_set_dir(shr16_get_dir() & ~4);
	int _speed = 3000;

	for (int i = 0; i < 400; i++)
	{
		PORTD |= 0x40;;
		asm("nop");
		PORTD &= ~0x40;
		asm("nop");
		delayMicroseconds(_speed);
	}

	isFilamentLoaded = true;  // filament loaded 
}

void unload_filament()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	shr16_set_dir(shr16_get_dir() | 4);
	int _speed = 1500;

	int i = 0; for (; i < 6480; i++) // 16323 -- 16500  --> shorter only for demo
	{
		PORTD |= 0x40;;
		asm("nop");
		PORTD &= ~0x40;
		asm("nop");

		if (i > 41 && i < 2000 && _speed > 260) _speed = _speed - 2;
		if (i > 4500 && _speed < 600) _speed = _speed + 1;
		if (i > 5000 && _speed < 1400) _speed = _speed + 1;
		delayMicroseconds(_speed);
	}
	isFilamentLoaded = false; // filament unloaded 
}

void park_idler(bool _unpark)
{
	if (_unpark) // get idler in contact with filament
	{
		move(idler_parking_steps, 0);
		isIdlerParked = false;
	} 
	else // park idler so filament can move freely
	{
		move(idler_parking_steps*-1, 0);
		isIdlerParked = true;
	}
}


bool home_idler()
{
	for (int c = 3; c > 0; c--)  // not really functional, let's do it rather more times to be sure
	{
		move(0, (c * 5) * -1);
		delay(50);
		shr16_set_dir(shr16_get_dir() | 1);
		for (int i = 0; i < 1000; i++)
		{
			move(1, 0);
			delay(5);
			uint16_t sg = tmc2130_read_sg(0);
			//printf_P(PSTR("SG=%d\n"), tmc2130_read_sg(0));
			if ((i > 16) && (sg < 100))
				break;
		}
	}
	return true;
}



bool home_selector()
{
	for (int c = 3; c > 0; c--)   // not really functional, let's do it rather more times to be sure
	{
		move(0, (c*20) * -1);
		delay(50);
		for (int i = 0; i < 4000; i++)
		{
			move(0, 1);
			uint16_t sg = tmc2130_read_sg(1);
			if ((i > 16) && (sg < 100))
				break;
		}
	}
	return true;
}



bool home()
{
	shr16_set_led(0x2aa);
	
	move(-10, -100); // move a bit in opposite direction
	
	// home both idler and selector
	home_idler();
	home_selector();
	
	shr16_set_led(0x155);

	move(idler_steps_after_homing, selector_steps_after_homing); // move to initial position

	active_extruder = 0;

	park_idler(false);
	shr16_set_led(0x000);
	
	isFilamentLoaded = false; 
	shr16_set_led(1 << 2 * active_extruder);
}



void move_filament(int _mm)
{
//	shr16_set_dir(shr16_get_dir() & ~4);
	int i = 0; for (; i < 4000; i++)
	{
//		tmc2130_do_step(4);
		delayMicroseconds(400);
	}
}


void move(int _idler, int _selector)
{
	int _acc = 50;

	// gets steps to be done and set direction
	_idler = set_idler_direction(_idler); 
	_selector = set_selector_direction(_selector);

	do
	{
		if (_idler > 0) { PORTB |= 0x10; _idler--; delayMicroseconds(5000); }
		if (_selector > 0) { PORTD |= 0x10; _selector--;  delayMicroseconds(800);}
		asm("nop");
		PORTD &= ~0x10;
		PORTB &= ~0x10;

		if (_acc > 0) { delayMicroseconds(_acc*10); _acc = _acc - 1; }; // super pseudo acceleration control

	} while (_selector != 0 || _idler != 0);
}





int set_idler_direction(int _steps)
{
	if (_steps < 0)
	{
		_steps = _steps * -1;
		shr16_set_dir(shr16_get_dir() & ~1);
	}
	else 
	{
		shr16_set_dir(shr16_get_dir() | 1);
	}
	return _steps;
}

int set_selector_direction(int _steps)
{
	if (_steps < 0)
	{
		_steps = _steps * -1;
		shr16_set_dir(shr16_get_dir() & ~2);
	}
	else
	{
		shr16_set_dir(shr16_get_dir() | 2);
	}
	return _steps;
}

