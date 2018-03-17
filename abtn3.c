//abtn3.c - 3 buttons on analog channel

#include "abtn3.h"
#include <Arduino.h>
//#include <avr/io.h>

uint8_t abtn3_sample(void)
{
	int raw = analogRead(BTN_APIN);
	// Button 1 - 0
	// Button 2 - 344
	// Button 3 - 516
	if (raw < 10) return 1;
	else if (raw > 300 && raw < 400) return 2;
	else if (raw > 450 && raw < 600) return 4;
	return(0);
}
