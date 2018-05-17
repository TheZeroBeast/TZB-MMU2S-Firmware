// 
// 
// 

#include "Buttons.h"
const int ButtonPin = A2;


int buttonClicked()
{
	int raw = analogRead(ButtonPin);
	int _return = 0;

	
	if (raw < 50) _return = 1;
	if (raw > 300 && raw < 400) _return = 2;
	if (raw > 480 && raw < 600) _return = 4;


	return(_return);
	

}

