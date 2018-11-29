// uart.h
#ifndef _UART_H
#define _UART_H

#define USART_BAUDRATE 115200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <inttypes.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "config.h"

extern bool rxFlag;
extern char rxBuf[BUFFER_SIZE];

#endif //_UART_H
