// uart.cpp

#include "uart.h"

volatile char rxData1, rxData2, rxData3, rxCSUM;
volatile bool startRxFlag, confirmedPayload, txNACK;

char lastTxPayload[3] = {0, 0, 0};

ISR(USART1_RX_vect)
{
  // NOTE: data1, data2, data3 and receivedchecksum must be volatile, global, etc

  // ACK = 0x7F       NACK = 0x15
  
  cli(); // disable global interrupts
  if (UDR1 == 0x7F && !startRxFlag) {// check for start of framing bytes 
    startRxFlag = true;  
  } else {
    if (startRxFlag == 1) {
      if (rxData1 > 0) {
        if (rxData2 > 0) {
          if (rxData3 > 0) {
            if (rxCSUM > 0) {
              if (UDR1 == 0x7F) {
                confirmedPayload = true; // set confirm payload bit true for processing my main loop
                return;
              } else {
                startRxFlag = 0;
                rxData1, rxData2, rxData3, rxCSUM = 0;
                txNACK = true; // **send universal nack here **
              }
            } else {
              rxCSUM = UDR1;
            }
          } else {
            rxData3 = UDR1;
          }
        } else {
          rxData2 = UDR1;
        }
      } else {
        rxData1 = UDR1;
      }
    } else {
      txNACK = true; // **send universal nack here **
      return; // do nothing at all if 0x7F isnt seen first
    }
  }
}

void txPayload(char payload[3])
{
  for (int i = 0; i < 3; i++) lastTxPayload[i] = payload[i];  // Backup incase resend on NACK
  char csum = 0x00;
  //loop_until_bit_is_set(UCSR1A, RXC1);
  UDR1 = 0x7F;                                                // Start byte
  for (int i = 0; i < 3; i++) {                               // Send data
    //loop_until_bit_is_set(UCSR1A, RXC1);
    UDR1 = payload[i];
    csum += payload[i];
  }
  //loop_until_bit_is_set(UCSR1A, RXC1);
  UDR1 = csum;                                                // Send Checksum
  //loop_until_bit_is_set(UCSR1A, RXC1);
  UDR1 = 0x7F;                                                // End byte
}
