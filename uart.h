// uart.h
#ifndef _UART_H
#define _UART_H

#include <inttypes.h>
#include <stdio.h>
#include "config.h"

extern FILE _uart0io;
#define uart0io (&_uart0io)

extern FILE _uart1io;
#define uart1io (&_uart1io)


void uart0_init(void);
void uart1_init(void);

#endif //_UART_H
