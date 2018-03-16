//uart.h
#ifndef _UART_H
#define _UART_H

#include <inttypes.h>
#include <stdio.h>
#include "config.h"


extern FILE _uart0out;
#define uart0out (&_uart0out)

extern FILE _uart1out;
#define uart1out (&_uart1out)


extern void uart0_init(void);

extern void uart1_init(void);


#endif //_UART_H
