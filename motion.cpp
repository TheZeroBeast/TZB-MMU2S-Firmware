#include "motion.h"
#include "shr16.h"
#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <Arduino.h>
#include "main.h"
#include "mmctl.h"
#include "Buttons.h"


const int selector_steps_after_homing = -3700;
const int idler_steps_after_homing = -130;

const int selector_steps = 2790/4;
const int idler_steps = 1420 / 4;    // 2 msteps = 180 / 4
const int idler_parking_steps = (idler_steps / 2) + 40;  // 40

const int bowden_length = 1000;

// endstop to tube  - 30 mm, 550 steps


void move(int _idler, int _selector, int _pulley);
void move_proportional(int _idler, int _selector);

int set_idler_direction(int _steps);
int set_selector_direction(int _steps);
int set_pulley_direction(int _steps);

void cut_filament();

void park_idler(bool _unpark);

void load_filament_inPrinter();
void load_filament_withSensor();

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
	move_proportional(_idler_steps, _selector_steps);
}



void load_filament_withSensor()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	tmc2130_init_axis_current(2, 1, 30);

	shr16_set_dir(shr16_get_dir() & ~4);

	int _loadSteps = 0;
	Serial.println("-----------------------------");
	Serial.print("LOAD ");
	Serial.println(active_extruder);

	// we can expect something like 570 steps to get in sensor
	do
	{
		do_pulley_step();
		_loadSteps++;
		delayMicroseconds(5500);

	} while (digitalRead(A1) == 0 && _loadSteps < 1200);
	
	Serial.print("  - steps :");
	Serial.println(_loadSteps);
	Serial.print("  - switch is :");
	Serial.println(digitalRead(A1));
	

	if (digitalRead(A1) == 0)
	{
		// attempt to correct
		Serial.println("!!!!!!!!!! Attempt to correct !!!!!!!!!!!");
		

		shr16_set_dir(shr16_get_dir() | 4);
		for (int i = 500; i >= 0; i--)
		{
			do_pulley_step();
			delayMicroseconds(6000);
		}

		shr16_set_dir(shr16_get_dir() & ~4);
		_loadSteps = 0;
		do
		{
			do_pulley_step();
			_loadSteps++;
			delayMicroseconds(6000);

		} while (digitalRead(A1) == 0 && _loadSteps < 1500);

		Serial.print("     - switch is :");
		Serial.print(digitalRead(A1));
		Serial.print("     - steps :");
		Serial.println(_loadSteps);
	}


	if (digitalRead(A1) == 0)
	{
		// error in loading
		Serial.println("ERROR on loading");
		Serial.print("  - switch is :");
		Serial.println(digitalRead(A1));

		park_idler(false);
		do
		{
			shr16_set_led(0x000);
			delay(100);
			shr16_set_led(1 << 1 * (4 - active_extruder));
			delay(100);
		} while ( buttonClicked() == 0 );

		park_idler(true);
		// TODO: do not repeat same code, try to do it until succesfull load
		_loadSteps = 0;
		do
		{
			do_pulley_step();
			_loadSteps++;
			delayMicroseconds(5500);

		} while (_loadSteps < 700 );
		// ?
	}
	else
	{
		Serial.print("  CORRECT, steps :");
		Serial.println(_loadSteps);
	}

	float _speed = 4500;

	for (int i = 0; i < 8500; i++)   // 8800
	{
		do_pulley_step();
		
		if (i > 10 && i < 4000 && _speed > 600) _speed = _speed - 4;
		if (i > 100 && i < 4000 && _speed > 600) _speed = _speed - 1;
		if (i > 8000 && _speed < 3000) _speed = _speed + 2;  
		delayMicroseconds(_speed);
	}

	tmc2130_init_axis_current(2, 0, 0);
	isFilamentLoaded = true;  // filament loaded 
}

void unload_filament_withSensor()
{
	// unloads filament from extruder - filament is above Bondtech gears
	tmc2130_init_axis_current(2, 1, 30);


	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	shr16_set_dir(shr16_get_dir() | 4);

	float _speed = 2000;
	float _first_point = 1800;
	float _second_point = 8700;  //8500
	int _endstop_hit = 0;
	Serial.println("-----------------------------");
	Serial.print("UNLOAD ");
	Serial.println(active_extruder);

	int _unloadSteps = 9500;
	do
	{
		do_pulley_step();
		_unloadSteps--;

		if (_unloadSteps < 1400 && _speed < 6000) _speed = _speed + 3;
		if (_unloadSteps < _first_point && _speed < 2500) _speed = _speed + 2;
		if (_unloadSteps < _second_point && _unloadSteps > 5000 && _speed > 550) _speed = _speed - 2;

		delayMicroseconds(_speed);
		if (digitalRead(A1) == 0 && _unloadSteps < 2500)
		{
			_endstop_hit++;
		}

	} while (_endstop_hit < 50 && _unloadSteps > 0);
	
	Serial.print(" - steps : ");
	Serial.println(_unloadSteps);

	Serial.print(" - switch is :");
	Serial.println(digitalRead(A1));

	for (int i = 150; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(5000);
	}


	if (digitalRead(A1) == 1)
	{
		for (int i = 10; i > 0; i--)
		{
			if (digitalRead(A1) == 1)
			{
				shr16_set_dir(shr16_get_dir() | 4);
				for (int i = 180; i > 0; i--)
				{
					do_pulley_step();
					delayMicroseconds(5000);
				}

				shr16_set_dir(shr16_get_dir() & ~4);
				for (int i = 150; i > 0; i--)
				{
					do_pulley_step();
					delayMicroseconds(5000);
				}
				
				shr16_set_dir(shr16_get_dir() | 4);
				int _steps = 4000;
				do
				{
					do_pulley_step();
					_steps--;
					delayMicroseconds(4000);
				} while (digitalRead(A1) == 1 && _steps > 0);
				Serial.println("!!!!!!!!!! Attempt to correct !!!!!!!!!!!");
				Serial.print("  -- steps to correct : ");
				Serial.println(_steps);
			}
			delay(100);
		}

		if (digitalRead(A1) == 0)
		{
			for (int i = 150; i > 0; i--)
			{
				do_pulley_step();
				delayMicroseconds(5000);
			}
		}

	}

	Serial.print(" - switch is :");
	Serial.println(digitalRead(A1));

	if (digitalRead(A1) == 1)
	{
		// error in unloading
		Serial.println("ERROR on unload");
		Serial.print(" - switch is :");
		Serial.println(digitalRead(A1));
		
		park_idler(false);
		do
		{
			shr16_set_led(0x000);
			shr16_set_led(1 << 2 * (4 - active_extruder));
			delay(80);
			shr16_set_led(1 << 2 * (4 - active_extruder));
			delay(80);
			shr16_set_led(0x000);
			delay(500);
		} while (buttonClicked() == 0);

		park_idler(true);
		
	}
	else
	{


		// correct unloading

		_speed = 5000;
		// unload to PTFE tube
		shr16_set_dir(shr16_get_dir() | 4);
		for (int i = 550; i > 0; i--)   // 570
		{
			do_pulley_step();
			delayMicroseconds(_speed);
		}

		_speed = 3000;
		// cooling move
		shr16_set_dir(shr16_get_dir() & ~4);
		for (int i = 150; i > 0; i--)
		{
			do_pulley_step();
			delayMicroseconds(_speed);
		}
		shr16_set_dir(shr16_get_dir() | 4);
		for (int i = 160; i > 0; i--)
		{
			do_pulley_step();
			delayMicroseconds(_speed);
		}
	}
	
	tmc2130_init_axis_current(2, 0, 0);
	isFilamentLoaded = false; // filament unloaded 
}

void load_filament_inPrinter()
{
	// loads filament after confirmed by printer into the Bontech pulley gears so they can grab them

	shr16_set_dir(shr16_get_dir() & ~4);

	/*PLA
	tmc2130_init_axis_current(2, 1, 15);   
	for (int i = 0; i <= 250; i++)
	{
		if (i == 125) { tmc2130_init_axis_current(2, 1, 10); };
		do_pulley_step();
		delayMicroseconds(2600);
	}
	*/

	//FLEX
	tmc2130_init_axis_current(2, 1, 15);
	for (int i = 0; i <= 500; i++)
	{
		if (i == 125) { tmc2130_init_axis_current(2, 1, 10); };
		do_pulley_step();
		delayMicroseconds(2600);
	}


	/*PLA
	tmc2130_init_axis_current(2, 1, 3);    
	for (int i = 0; i <= 400; i++)
	{
		do_pulley_step();
		delayMicroseconds(2200);   //3200
	}
	*/

	// FLEX
	tmc2130_init_axis_current(2, 1, 3);
	for (int i = 0; i <= 800; i++)
	{
		do_pulley_step();
		delayMicroseconds(2000);   //3200
	}


	tmc2130_init_axis_current(2, 0, 0);
	park_idler(false);
	isIdlerParked = true;
}

void init_Pulley()
{
	float _speed = 4000;
	

	shr16_set_dir(shr16_get_dir() & ~4);
	for (int i = 200; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(_speed);
		shr16_set_led(1 << 2 * (int)(i/50));
	}

	shr16_set_dir(shr16_get_dir() | 4);
	for (int i = 200; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(_speed);
		shr16_set_led(1 << 2 * (4-(int)(i / 50)));
	}

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
		}
	}
	return true;
}

bool home_selector()
{
	 
	for (int c = 5; c > 0; c--)   // not really functional, let's do it rather more times to be sure
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
 



void move_proportional(int _idler, int _selector)
{
	// gets steps to be done and set direction
	_idler = set_idler_direction(_idler);
	_selector = set_selector_direction(_selector);

	float _idler_step = (float)_idler/(float)_selector;
	float _idler_pos = 0;
	int _speed = 2500;
	int _start = _selector - 250;
	int _end = 250;

	do
	{
		if (_idler_pos >= 1)
		{
			if (_idler > 0) { PORTB |= 0x10; }
		}
		if (_selector > 0) { PORTD |= 0x10; }
		
		asm("nop");
		
		if (_idler_pos >= 1)
		{
			if (_idler > 0) { PORTB &= ~0x10; _idler--;  }
		}

		if (_selector > 0) { PORTD &= ~0x10; _selector--; }
		asm("nop");

		if (_idler_pos >= 1)
		{
			_idler_pos = _idler_pos - 1;
		}


		_idler_pos = _idler_pos + _idler_step;

		delayMicroseconds(_speed);
		if (_speed > 900 && _selector > _start) { _speed = _speed - 10; }
		if (_speed < 2500 && _selector < _end) { _speed = _speed + 10; }

	} while (_selector != 0 || _idler != 0 );
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
