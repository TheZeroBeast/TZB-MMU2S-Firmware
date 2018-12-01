// uart.h
#ifndef _UART_H
#define _UART_H

#define USART_BAUDRATE  115200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define NACK          (char)0x150000 // ASCII for NACK
#define NACKCheck     (char)0x15     // ASCII for NACK Checking
#define OK            (char)0x4F4B00 // ASCII for  OK CMD tx
#define OKd           (char)0x4F4B   // ASCII for  OK CMD tx. For use when sending a data byte
#define BLK           (char)0x00     // Blank data filler

#include <inttypes.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Arduino.h"
#include "config.h"


extern volatile char rxData1, rxData2, rxData3, rxCSUM;
extern volatile bool confirmedPayload, txNACK;

extern char lastTxPayload[3];

void txPayload(char payload[3]);

#endif //_UART_H
