// uart.h
#ifndef _UART_H
#define _UART_H

#define USART1_BAUDRATE  115200UL
#define MMU_F_CPU       16000000UL
#define BAUD_PRESCALE (((MMU_F_CPU / (USART1_BAUDRATE * 16UL))) - 1)
#define ACK           "ACK"
#define NAK           "NAK"
//#define NACKCheck     (char)0x15     // ASCII for NACK Checking
#define OK            0x4F4B00 // ASCII for  OK CMD tx
#define OKd           0x4F4B   // ASCII for  OK CMD tx. For use when sending a data byte
#define BLK           (char)0x2D     // Blank data filler

#include <inttypes.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Arduino.h"
#include "config.h"

//For comms testing
#include "shr16.h"


extern volatile unsigned char rxData1, rxData2, rxData3, rxCSUM;
extern volatile bool startRxFlag, confirmedPayload, txNACK;
extern bool pendingACK;

extern char lastTxPayload[3];

void txPayload(unsigned char payload[3]);

#endif //_UART_H
