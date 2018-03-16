//uart.cpp

#include "uart.h"
#include "Arduino.h"


FILE _uart0out = {0};

FILE _uart1out = {0};


int uart0_putchar(char c, FILE *stream)
{
	Serial.write(c);
	return 0;
}

int uart1_putchar(char c, FILE *stream)
{
	Serial1.write(c);
	return 0;
}


void uart0_init(void)
{
	Serial.begin(UART0_BDR, SERIAL_8N2); //serial0 - USB
	fdev_setup_stream(uart0out, uart0_putchar, NULL, _FDEV_SETUP_WRITE); //setup uart out stream
}

void uart1_init(void)
{
	Serial1.begin(UART1_BDR, SERIAL_8N2); //serial1
	fdev_setup_stream(uart1out, uart1_putchar, NULL, _FDEV_SETUP_WRITE); //setup uart out stream
}
