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
const int idler_steps_after_homing = -130;

const int selector_steps = 2790/4;
const int idler_steps = 1420 / 4;    // 2 msteps = 180 / 4
const int idler_parking_steps = (idler_steps / 2)+40;  // 

const int bowden_length = 1000;




void move(int _idler, int _selector, int _pulley);

int set_idler_direction(int _steps);
int set_selector_direction(int _steps);
int set_pulley_direction(int _steps);

void cut_filament();

void park_idler(bool _unpark);
void load_filament();
void unload_filament();
void load_filament_inPrinter();

void do_pulley_step();
void do_idler_step();

void set_positions(int _current_extruder, int _next_extruder);



void cut_filament()
{
}



void set_positions(int _current_extruder, int _next_extruder)
{
	// steps to move to new position of idler and selector
	int _selector_steps = ((_current_extruder - _next_extruder) * selector_steps) * -1;
	int _idler_steps = (_current_extruder - _next_extruder) * idler_steps;

	// move both to new position
	move(_idler_steps, _selector_steps,0);
}

void load_filament()
{
	// loads filament from parking position into the extruder and stops above Bondtech gears.
	// Rest done after printer confirms by "OK"

	// 1 step = 0.049 mm
	// bowden - PTFE length = 330 mm
	// 60 mm to pass first Festo ( 1220 steps )
	// 7800 steps to get out from second FESTO
	// 35 mm from second FESTO to bondtech gears ( 720 steps )
	// 8550 steps total length

	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	shr16_set_dir(shr16_get_dir() & ~4);

	float _speed = 5500;

	for (int i = 0; i < 9450; i++)   // 8800
	{
		do_pulley_step();
		
		if (i > 200 && i < 4000 && _speed > 600) _speed = _speed - 3;
		if (i > 100 && i < 4000 && _speed > 600) _speed = _speed - 1;
		if (i > 8300 && _speed < 3000) _speed = _speed + 2;  
		delayMicroseconds(_speed);
	}
	
	isFilamentLoaded = true;  // filament loaded 
}

 


void unload_filament()
{
	// unloads filament from extruder - filament is above Bondtech gears

	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	shr16_set_dir(shr16_get_dir() | 4);

	float _speed = 2000;
	float _first_point = 1100;
	float _second_point = 9300;  //8500

	for (int i = 9400; i > 0; i--)    //8950
	{
		do_pulley_step();

		if (i < 400 && _speed < 6000) _speed = _speed + 3;
		if (i < _first_point && _speed < 2500) _speed = _speed + 2;
		if (i < _second_point && i > 5000 &&  _speed > 600) _speed = _speed - 2;

		delayMicroseconds(_speed);
	}

	// move filament here and there to settle down in cooling PTFE tube
	_speed = 8000;
	shr16_set_dir(shr16_get_dir() & ~4);
	for (int i = 150; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(_speed);
	}
	shr16_set_dir(shr16_get_dir() | 4);
	for (int i = 140; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(_speed);
	}
	isFilamentLoaded = false; // filament unloaded 
}




void load_filament_inPrinter()
{
	// loads filament after confirmed by printer into the Bontech pulley gears so they can grab them

	shr16_set_dir(shr16_get_dir() & ~4);
	
	for (int i = 0; i <= 180; i++)
	{
		do_pulley_step();
		delayMicroseconds(3400);
	}

	// last steps done with releasing idler
	move(idler_parking_steps*-1, 0, idler_parking_steps);
	
	isIdlerParked = true;
}




void do_pulley_step()
{
	PORTD |= 0x40;;
	asm("nop");
	PORTD &= ~0x40;
	asm("nop");
}



void do_idler_step()
{
	PORTB |= 0x10;;
	asm("nop");
	PORTB &= ~0x10;
	asm("nop");
}


void park_idler(bool _unpark)
{

	if (_unpark) // get idler in contact with filament
	{
		move(idler_parking_steps, 0,0);
		isIdlerParked = false;
	} 
	else // park idler so filament can move freely
	{
		move(idler_parking_steps*-1, 0,0);
		isIdlerParked = true;
	}
	 
}


bool home_idler()
{
	for (int c = 1; c > 0; c--)  // not really functional, let's do it rather more times to be sure
	{
		move(0, (c * 5) * -1,0);
		delay(50);
		shr16_set_dir(shr16_get_dir() | 1);
		for (int i = 0; i < 2000; i++)
		{
			move(1, 0,0);
			delayMicroseconds(100);
			uint16_t sg = tmc2130_read_sg(0);
			//printf_P(PSTR("SG=%d\n"), tmc2130_read_sg(0));
			//if ((i > 2) && (sg < 100))	break;
		}
	}
	return true;
}
bool home_selector()
{
	 
	for (int c = 3; c > 0; c--)   // not really functional, let's do it rather more times to be sure
	{
		move(0, (c*20) * -1,0);
		delay(50);
		for (int i = 0; i < 4000; i++)
		{
			move(0, 1,0);
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
	
	move(-10, -100,0); // move a bit in opposite direction
	
	// home both idler and selector
	home_idler();
	home_selector();
	
	shr16_set_led(0x155);

	move(idler_steps_after_homing, selector_steps_after_homing,0); // move to initial position

	active_extruder = 0;

	park_idler(false);
	shr16_set_led(0x000);
	
	isFilamentLoaded = false; 
	shr16_set_led(1 << 2 * (4-active_extruder));

  isHomed = true;
}






void move(int _idler, int _selector, int _pulley)
{
	int _acc = 50;

	// gets steps to be done and set direction
	_idler = set_idler_direction(_idler); 
	_selector = set_selector_direction(_selector);
	_pulley = set_pulley_direction(_pulley);
	

	do
	{
		if (_idler > 0) { PORTB |= 0x10;}
		if (_selector > 0) { PORTD |= 0x10;}
		if (_pulley > 0) { PORTD |= 0x40; }
		asm("nop");
		if (_idler > 0) { PORTB &= ~0x10; _idler--; delayMicroseconds(1000); }
		if (_selector > 0) { PORTD &= ~0x10; _selector--;  delayMicroseconds(800); }
		if (_pulley > 0) { PORTD &= ~0x40; _pulley--;  delayMicroseconds(700); }
		asm("nop");

		if (_acc > 0) { delayMicroseconds(_acc*10); _acc = _acc - 1; }; // super pseudo acceleration control

	} while (_selector != 0 || _idler != 0 || _pulley != 0);
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

int set_pulley_direction(int _steps)
{
	if (_steps < 0)
	{
		_steps = _steps * -1;
		shr16_set_dir(shr16_get_dir() | 4);
	}
	else
	{
		shr16_set_dir(shr16_get_dir() & ~4);
	}
	return _steps;
}
