// uart.cpp

#include "uart.h"

volatile unsigned char readRxBuffer, rxData1, rxData2, rxData3, rxCSUM;
volatile bool startRxFlag = false, confirmedPayload, txNACK;
bool pendingACK = false;

char lastTxPayload[3] = {0, 0, 0};

ISR(USART1_RX_vect)
{
  cli();
  readRxBuffer = UDR1;
  if ((readRxBuffer == 0x7F) && (!startRxFlag)) {// check for start of framing bytes
    startRxFlag = true;
  } else if (startRxFlag == true) {
      if (rxData1 > 0) {
        if (rxData2 > 0) {
          if (rxData3 > 0) {
            if (rxCSUM > 0) {
              if (readRxBuffer == 0x7F) {
                confirmedPayload = true; // set confirm payload bit true for processing my main loop
              } else {
                startRxFlag = false;
                rxData1 = 0;
                rxData2 = 0;
                rxData3 = 0;
                rxCSUM = 0;
                txNACK = true; // **send universal nack here **
              }
              shr16_set_led(0xFFFF);
            } else {
              rxCSUM = readRxBuffer;
            }
          } else {
            rxData3 = readRxBuffer;
          }
        } else {
          rxData2 = readRxBuffer;
        }
      } else {
        rxData1 = readRxBuffer;
      }
    }// else {
      //startRxFlag = false;
      //rxData1, rxData2, rxData3, rxCSUM = 0; // Clear rx vars
      //txNACK = true; // **send universal nack here **
    //}
  sei();
}
void txPayload(unsigned char payload[3])
{
  for (int i = 0; i < 3; i++) lastTxPayload[i] = payload[i];  // Backup incase resend on NACK
  uint8_t csum = 0;
  loop_until_bit_is_set(UCSR1A, UDRE1); // Check is UDRE is that and not something with a 2 in it - Do nothing until UDR is ready for more data to be written to it
  UDR1 = 0x7F;                                                // Start byte 0x7F
  delay(10);
  for (int i = 0; i < 3; i++) {                               // Send data
    loop_until_bit_is_set(UCSR1A, UDRE1); // Check is UDRE is that and not something with a 2 in it - Do nothing until UDR is ready for more data to be written to it
    UDR1 = (0xFF & payload[i]);
    delay(10);
    csum += (0xFF & payload[i]);
  }
  csum = 0x2D; //(csum/3);
  loop_until_bit_is_set(UCSR1A, UDRE1); // Check is UDRE is that and not something with a 2 in it - Do nothing until UDR is ready for more data to be written to it
  UDR1 = (0xFF & csum);
  delay(10);// Send Checksum
  loop_until_bit_is_set(UCSR1A, UDRE1); // Check is UDRE is that and not something with a 2 in it - Do nothing until UDR is ready for more data to be written to it
  UDR1 = 0x7F;
  delay(10);// End byte
  if ((payload != ACK) || (payload != NAK)) pendingACK = true;                                          // Set flag to wait for ACK
}
