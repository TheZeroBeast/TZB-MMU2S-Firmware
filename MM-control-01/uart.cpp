// uart.cpp

#include "uart.h"

volatile unsigned char readRxBuffer, rxData1 = 0, rxData2 = 0, rxData3 = 0,
  rxCSUM1 = 0, rxCSUM2 = 0;
volatile bool startRxFlag = false, confirmedPayload = false, txNAKNext = false,
  txACKNext = false, txRESEND = false, pendingACK = false;
volatile uint8_t rxCount;

byte lastTxPayload[3] = {0, 0, 0};

ISR(USART1_RX_vect)
{
  cli();
  readRxBuffer = UDR1;
  if ((readRxBuffer == 0x7F) && (!startRxFlag)) {// check for start of framing bytes
    startRxFlag = true;
    rxCount = 0;
  } else if (startRxFlag == true) {
    if (rxCount > 0) {
      if (rxCount > 1) {
        if (rxCount > 2) {
          if (rxCount > 3) {
            if (rxCount > 4) {
              if (readRxBuffer == 0xF7) {
                  confirmedPayload = true; // set confirm payload bit true for processing my main loop
              } else txNAKNext = true; // **send universal nack here **
            } else {
              rxCSUM2 = readRxBuffer;
              ++rxCount;
            }
          } else {
            rxCSUM1 = readRxBuffer;
            ++rxCount;
          }
        } else {
          rxData3 = readRxBuffer;
          ++rxCount;
        }
      } else {
        rxData2 = readRxBuffer;
        ++rxCount;
      }
    } else {
      rxData1 = readRxBuffer;
      ++rxCount;
    }
  } else if (readRxBuffer == 0x06) pendingACK = false;  // ACK Received Clear pending flag
  else   if (readRxBuffer == 0x15) txRESEND = true;     // Resend last message
  sei();
}

void txPayload(unsigned char payload[3])
{
  for (int i = 0; i < 3; i++) lastTxPayload[i] = payload[i];  // Backup incase resend on NACK
  uint16_t csum = 0;
  loop_until_bit_is_set(UCSR1A, UDRE1);   // Do nothing until UDR is ready for more data to be written to it
  UDR1 = 0x7F;                            // Start byte 0x7F
  //delay(4);
  for (int i = 0; i < 3; i++) {           // Send data
    loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
    UDR1 = payload[i];
    csum += payload[i];
    //delay(4);
  }
  loop_until_bit_is_set(UCSR1A, UDRE1);   // Do nothing until UDR is ready for more data to be written to it
  UDR1 = ((0xFFFF & csum) >> 8);
  //delay(4);
  loop_until_bit_is_set(UCSR1A, UDRE1);   // Do nothing until UDR is ready for more data to be written to it
  UDR1 = (0xFF & csum);
  //delay(4);
  loop_until_bit_is_set(UCSR1A, UDRE1);   // Do nothing until UDR is ready for more data to be written to it
  UDR1 = 0xF7;
  //delay(4);
  pendingACK = true;                      // Set flag to wait for ACK
}

void txACK(bool ACK)
{
  confirmedPayload = false;
  startRxFlag      = false;
  if (ACK) {
    loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0x06; // ACK HEX
    txACKNext = false;
  } else {
    loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0x15; // NACK HEX
    txNAKNext = false;
  }
}
