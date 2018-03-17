
#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "spi.h"
#include "tmc2130.h"
#include "mmctl.h"


bool home_idler()
{
	shr16_set_dir(shr16_get_dir() & ~1);
	int i = 0; for (; i < 4000; i++)
	{
		tmc2130_do_step(1);
		delay(1);
		uint16_t sg = tmc2130_read_sg(0);
		if ((i > 16) && (sg < 100))
			break;
		printf_P(PSTR("SG=%d\n"), tmc2130_read_sg(0));
	}
	return true;
}

bool home_selector()
{
//	shr16_set_dir(shr16_get_dir() & ~2);
	shr16_set_dir(shr16_get_dir() | 2);
	int i = 0; for (; i < 4000; i++)
	{
		tmc2130_do_step(2);
		delay(1);
		uint16_t sg = tmc2130_read_sg(1);
		if ((i > 16) && (sg < 100))
			break;
		printf_P(PSTR("SG=%d\n"), tmc2130_read_sg(1));
	}
	return (i < 4000);
}

bool move_selector()
{
	shr16_set_dir(shr16_get_dir() & ~2);
//	shr16_set_dir(shr16_get_dir() | 2);
	for (int i = 0; i < 2000; i++)
	{
		tmc2130_do_step(2);
		delay(1);
	}
	return true;
}

bool home_()
{
	shr16_set_dir(shr16_get_dir() & ~2);
	int i = 0; for (; i < 3000; i++)
	{
		tmc2130_do_step(2);
		delay(1);
	}
	return true;
}

bool switch_extruder(int new_extruder)
{
	//TODO - control motors
	active_extruder = new_extruder;
	shr16_set_led(1 << 2*active_extruder);
	return true;
}
