// uart.h
#ifndef _UART_H
#define _UART_H

#define USART1_BAUDRATE  38400UL
#define MMU2S_F_CPU       16000000UL
#define BAUD_PRESCALE (((MMU2S_F_CPU / (USART1_BAUDRATE * 16UL))) - 1)
#define OK            (unsigned char*)"OK---" // 'OK-' ASCII for  OK CMD tx
#define BLK           0x2D  // Blank data filler

#include <inttypes.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Arduino.h"
#include "config.h"

extern volatile unsigned char rxData1, rxData2, rxData3, rxData4, rxData5;
extern volatile bool confirmedPayload, txNAKNext, txACKNext,
       txRESEND, pendingACK, IR_SENSOR;
extern unsigned long startTXTimeout;

extern unsigned char lastTxPayload[5];

extern void txPayload(unsigned char*, bool retry = false);
extern void txACK(bool ACK = true);
extern void txACKMessageCheck(void);

#endif //_UART_H