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

int selector_steps_for_eject = 0;
int idler_steps_for_eject = 0;

int8_t filament_type[EXTRUDERS] = {-1, -1, -1, -1, -1};

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

bool checkOk();

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

void eject_filament(int extruder)
{
	//move selector sideways and push filament forward little bit, so user can catch it, unpark idler at the end to user can pull filament out

	int selector_position = 0;
	int steps = 0;

	int8_t selector_offset_for_eject = 0;
	int8_t idler_offset_for_eject = 0;
		
	//if there is still filament detected by PINDA unload it first
	if (isFilamentLoaded)  unload_filament_withSensor();
	
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	tmc2130_init_axis_current(0, 1, 30);
		
	//if we are want to eject fil 0-2, move seelctor to position 4 (right), if we want to eject filament 3 - 4, move selector to position 0 (left)
	//maybe we can also move selector to service position in the future?
	if (extruder <= 2) selector_position = 4;
	else selector_position = 0;

	//count offset (number of positions) for desired selector and idler position for ejecting
	selector_offset_for_eject = active_extruder - selector_position;
	idler_offset_for_eject = active_extruder - extruder;

	//count number of desired steps for selector and idler and store it in static variable
	selector_steps_for_eject = (selector_offset_for_eject * selector_steps) * -1;
	idler_steps_for_eject = idler_offset_for_eject * idler_steps;

	//move selector and idler to new position
	move_proportional(idler_steps_for_eject, selector_steps_for_eject);

	//push filament forward
	do
	{
		do_pulley_step();
		steps++;
		delayMicroseconds(1500);
	} while (steps < 2500);

	//unpark idler so user can easily remove filament
	park_idler(false);
	tmc2130_init_axis_current(0, 0, 0);
}

void recover_after_eject()
{
	//restore state before eject filament
	//if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	tmc2130_init_axis_current(0, 1, 30);
	move_proportional(-idler_steps_for_eject, -selector_steps_for_eject);
	tmc2130_init_axis_current(0, 0, 0);
	//unpark idler
	//park_idler(false);
}

void load_filament_withSensor()
{
	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	tmc2130_init_axis_current(0, 1, 30);

	set_pulley_dir_push();

	int _loadSteps = 0;
	int _endstop_hit = 0;

	// load filament until FINDA senses end of the filament, means correctly loaded into the selector
	// we can expect something like 570 steps to get in sensor
	do
	{
		do_pulley_step();
		_loadSteps++;
		delayMicroseconds(5500);
	} while (digitalRead(A1) == 0 && _loadSteps < 1500);


	// filament did not arrived at FINDA, let's try to correct that
	if (digitalRead(A1) == 0)
	{
		for (int i = 6; i > 0; i--)
		{
			if (digitalRead(A1) == 0)
			{
				// attempt to correct
				set_pulley_dir_pull();
				for (int i = 200; i >= 0; i--)
				{
					do_pulley_step();
					delayMicroseconds(1500);
				}

				set_pulley_dir_push();
				_loadSteps = 0;
				do
				{
					do_pulley_step();
					_loadSteps++;
					delayMicroseconds(4000);
					if (digitalRead(A1) == 1) _endstop_hit++;
				} while (_endstop_hit<100 && _loadSteps < 500);
			}
		}
	}

	// still not at FINDA, error on loading, let's wait for user input
	if (digitalRead(A1) == 0)
	{
		bool _continue = false;
		bool _isOk = false;



		park_idler(false);
		do
		{
			shr16_set_led(0x000);
			delay(800);
			if (!_isOk)
			{
				shr16_set_led(2 << 2 * (4 - active_extruder));
			}
			else
			{
				shr16_set_led(1 << 2 * (4 - active_extruder));
				delay(100);
				shr16_set_led(2 << 2 * (4 - active_extruder));
				delay(100);
			}
			delay(800);

			switch (buttonClicked())
			{
				case 4:
					// just move filament little bit
					park_idler(true);
					set_pulley_dir_push();

					for (int i = 0; i < 200; i++)
					{
						do_pulley_step();
						delayMicroseconds(5500);
					}
					park_idler(false);
					break;
				case 2:
					// check if everything is ok
					park_idler(true);
					_isOk = checkOk();
					park_idler(false);
					break;
				case 1:
					// continue with loading
					park_idler(true);
					_isOk = checkOk();
					park_idler(false);

					if (_isOk) //pridat do podminky flag ze od tiskarny prislo continue
					{
						_continue = true;
					}
					break;
			}
			
		} while ( !_continue );

		
		
		
		
		
		park_idler(true);
		// TODO: do not repeat same code, try to do it until succesfull load
		_loadSteps = 0;
		do
		{
			do_pulley_step();
			_loadSteps++;
			delayMicroseconds(5500);
		} while (digitalRead(A1) == 0 && _loadSteps < 1500);
		// ?
	}
	else
	{
		// nothing
	}

	float _speed = 4500;

	for (int i = 0; i < 7900 + ( lengthCorrection * 10 ); i++)   
	{
		do_pulley_step();
		
		if (i > 10 && i < 4000 && _speed > 650) _speed = _speed - 4;
		if (i > 100 && i < 4000 && _speed > 650) _speed = _speed - 1;
		if (i > 8000 && _speed < 3000) _speed = _speed + 2;  
		delayMicroseconds(_speed);
	}

	tmc2130_init_axis_current(0, 0, 0);
	isFilamentLoaded = true;  // filament loaded 
}

void unload_filament_withSensor()
{
	// unloads filament from extruder - filament is above Bondtech gears
	tmc2130_init_axis_current(0, 1, 30);


	if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament

	set_pulley_dir_pull();

	float _speed = 2000;
	float _first_point = 1800;
	float _second_point = 8700;   
	int _endstop_hit = 0;


	// unload until FINDA senses end of the filament
	int _unloadSteps = 10000;
	do
	{
		do_pulley_step();
		_unloadSteps--;

		if (_unloadSteps < 1400 && _speed < 6000) _speed = _speed + 3;
		if (_unloadSteps < _first_point && _speed < 2500) _speed = _speed + 2;
		if (_unloadSteps < _second_point && _unloadSteps > 5000 && _speed > 550) _speed = _speed - 2;

		delayMicroseconds(_speed);
		if (digitalRead(A1) == 0 && _unloadSteps < 2500) _endstop_hit++;

	} while (_endstop_hit < 100 && _unloadSteps > 0);

	// move a little bit so it is not a grinded hole in filament
	for (int i = 100; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(5000);
	}



	// FINDA is still sensing filament, let's try to unload it once again
	if (digitalRead(A1) == 1)
	{
		for (int i = 6; i > 0; i--)
		{
			if (digitalRead(A1) == 1)
			{
				set_pulley_dir_push();
				for (int i = 150; i > 0; i--)
				{
					do_pulley_step();
					delayMicroseconds(4000);
				}

				set_pulley_dir_pull();
				int _steps = 4000;
				_endstop_hit = 0;
				do
				{
					do_pulley_step();
					_steps--;
					delayMicroseconds(3000);
					if (digitalRead(A1) == 0) _endstop_hit++;
				} while (_endstop_hit < 100 && _steps > 0);
			}
			delay(100);
		}

	}



	// error, wait for user input
	if (digitalRead(A1) == 1)
	{
		bool _continue = false;
		bool _isOk = false;

		park_idler(false);
		do
		{
			shr16_set_led(0x000);
			delay(100);
			if (!_isOk)
			{
				shr16_set_led(2 << 2 * (4 - active_extruder));
			}
			else
			{
				shr16_set_led(1 << 2 * (4 - active_extruder));
				delay(100);
				shr16_set_led(2 << 2 * (4 - active_extruder));
				delay(100);
			}
			delay(100);


			switch (buttonClicked())
			{
			case 4:
				// just move filament little bit
				park_idler(true);
				set_pulley_dir_pull();

				for (int i = 0; i < 200; i++)
				{
					do_pulley_step();
					delayMicroseconds(5500);
				}
				park_idler(false);
				break;
			case 2:
				// check if everything is ok
				park_idler(true);
				_isOk = checkOk();
				park_idler(false);
				break;
			case 1:
				// continue with unloading
				park_idler(true);
				_isOk = checkOk();
				park_idler(false);

				if (_isOk)
				{
					_continue = true;
				}
				break;
			}


		} while (!_continue);
		
		shr16_set_led(1 << 2 * (4 - previous_extruder));
		park_idler(true);
	}
	else
	{
		// correct unloading
		_speed = 5000;
		// unload to PTFE tube
		set_pulley_dir_pull();
		for (int i = 450; i > 0; i--)   // 570
		{
			do_pulley_step();
			delayMicroseconds(_speed);
		}
	}
	park_idler(false);
	tmc2130_init_axis_current(0, 0, 0);
	isFilamentLoaded = false; // filament unloaded 
}

void load_filament_inPrinter()
{
	// loads filament after confirmed by printer into the Bontech pulley gears so they can grab them

  if (isIdlerParked) park_idler(true); // if idler is in parked position un-park him get in contact with filament
	set_pulley_dir_push();

	//PLA
	tmc2130_init_axis_current(0, 1, 15);   
	for (int i = 0; i <= 320; i++)
	{
		if (i == 150) { tmc2130_init_axis_current(0, 1, 10); };
		do_pulley_step();
		delayMicroseconds(2600);
	}

	//PLA
	tmc2130_init_axis_current(0, 1, 3);    
	for (int i = 0; i <= 450; i++)
	{
		do_pulley_step();
		delayMicroseconds(2200); 
	}
		
	park_idler(false);
	tmc2130_init_axis_current(0, 0, 0);
}

void init_Pulley()
{
	float _speed = 3000;
	

	set_pulley_dir_push();
	for (int i = 50; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(_speed);
		shr16_set_led(1 << 2 * (int)(i/50));
	}

	set_pulley_dir_pull();
	for (int i = 50; i > 0; i--)
	{
		do_pulley_step();
		delayMicroseconds(_speed);
		shr16_set_led(1 << 2 * (4-(int)(i / 50)));
	}

}

void do_pulley_step()
{
	PORTB |= 0x10;
	asm("nop");
	PORTB &= ~0x10;
	asm("nop");
}

void do_idler_step()
{
	PORTD |= 0x40;
	asm("nop");
	PORTD &= ~0x40;
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
	int _c = 0;
	int _l = 0;

	for (int c = 1; c > 0; c--)  // not really functional, let's do it rather more times to be sure
	{
		move(0, (c * 5) * -1,0);
		delay(50);
		for (int i = 0; i < 2000; i++)
		{
			move(1, 0,0);
			delayMicroseconds(100);
			uint16_t sg = tmc2130_read_sg(0);

			_c++;
			if (i == 1000) { _l++; }
			if (_c > 100) { shr16_set_led(1 << 2 * _l); };
			if (_c > 200) { shr16_set_led(0x000); _c = 0; };
		}
	}
	return true;
}

bool home_selector()
{
	 
	int _c = 0;
	int _l = 2;

	for (int c = 5; c > 0; c--)   // not really functional, let's do it rather more times to be sure
	{
		move(0, (c*20) * -1,0);
		delay(50);
		for (int i = 0; i < 4000; i++)
		{
			move(0, 1,0);
			uint16_t sg = tmc2130_read_sg(1);
			if ((i > 16) && (sg < 10))	break;

			_c++;
			if (i == 3000) { _l++; }
			if (_c > 100) { shr16_set_led(1 << 2 * _l); };
			if (_c > 200) { shr16_set_led(0x000); _c = 0; };
		}
	}
	
	return true;
}

bool home()
{
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
			if (_idler > 0) { PORTD |= 0x40; }
		}
		if (_selector > 0) { PORTD |= 0x10; }
		
		asm("nop");
		
		if (_idler_pos >= 1)
		{
			if (_idler > 0) { PORTD &= ~0x40; _idler--;  }
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
		if (_idler > 0) { PORTD |= 0x40; }
		if (_selector > 0) { PORTD |= 0x10;}
		if (_pulley > 0) { PORTB |= 0x10; }
		asm("nop");
		if (_idler > 0) { PORTD &= ~0x40; _idler--; delayMicroseconds(1000); }
		if (_selector > 0) { PORTD &= ~0x10; _selector--;  delayMicroseconds(800); }
		if (_pulley > 0) { PORTB &= ~0x10; _pulley--;  delayMicroseconds(700); }
		asm("nop");

		if (_acc > 0) { delayMicroseconds(_acc*10); _acc = _acc - 1; }; // super pseudo acceleration control

	} while (_selector != 0 || _idler != 0 || _pulley != 0);
}


void set_idler_dir_down()
{
	shr16_set_dir(shr16_get_dir() & ~4);
	//shr16_set_dir(shr16_get_dir() | 4);
}
void set_idler_dir_up()
{
	shr16_set_dir(shr16_get_dir() | 4);
	//shr16_set_dir(shr16_get_dir() & ~4);
}


int set_idler_direction(int _steps)
{
	if (_steps < 0)
	{
		_steps = _steps * -1;
		set_idler_dir_down();
	}
	else 
	{
		set_idler_dir_up();
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
		set_pulley_dir_pull();
	}
	else
	{
		set_pulley_dir_push();
	}
	return _steps;
}

void set_pulley_dir_push()
{
	shr16_set_dir(shr16_get_dir() & ~1);
}
void set_pulley_dir_pull()
{
	shr16_set_dir(shr16_get_dir() | 1);
}


bool checkOk()
{
	bool _ret = false;
	int _steps = 0;
	int _endstop_hit = 0;


	// filament in FINDA, let's try to unload it
	set_pulley_dir_pull();
	if (digitalRead(A1) == 1)
	{
		_steps = 3000;
		_endstop_hit = 0;
		do
		{
			do_pulley_step();
			delayMicroseconds(3000);
			if (digitalRead(A1) == 0) _endstop_hit++;
			_steps--;
		} while (_steps > 0 && _endstop_hit < 50);
	}

	if (digitalRead(A1) == 0)
	{
		// looks ok, load filament to FINDA
		set_pulley_dir_push();
		
		_steps = 3000;
		_endstop_hit = 0;
		do
		{
			do_pulley_step();
			delayMicroseconds(3000);
			if (digitalRead(A1) == 1) _endstop_hit++;
			_steps--;
		} while (_steps > 0 && _endstop_hit < 50);

		if (_steps == 0)
		{
			// we ran out of steps, means something is again wrong, abort
			_ret = false;
		}
		else
		{
			// looks ok !
			// unload to PTFE tube
			set_pulley_dir_pull();
			for (int i = 600; i > 0; i--)   // 570
			{
				do_pulley_step();
				delayMicroseconds(3000);
			}
			_ret = true;
		}

	}
	else
	{
		// something is wrong, abort
		_ret = false;
	}

	return _ret;
}
