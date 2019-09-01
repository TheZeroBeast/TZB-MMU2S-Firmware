// uart.cpp
#include "uart.h"

volatile unsigned char readRxBuffer, rxData1 = 0, rxData2 = 0, rxData3 = 0,
                                     rxData4 = 0, rxData5 = 0;
volatile bool confirmedPayload = false, txNAKNext = false,
              txACKNext = false, txRESEND = false, pendingACK = false, IR_SENSOR = false;
enum class rx
{
    Idle,
    Data1,
    Data2,
    Data3,
    Data4,
    Data5,
    End
};

inline rx& operator++(rx& byte, int)
{
    const int i = static_cast<int>(byte) + 1;
    byte = static_cast<rx>((i) % 7);
    return byte;
}

rx rxCount = rx::Idle;
unsigned char lastTxPayload[] = {0, 0, 0, 0, 0}; // used to try resend once

ISR(USART1_RX_vect)
{
    cli();
    readRxBuffer = UDR1;
    switch (rxCount) {
    case rx::Idle:
        if (readRxBuffer == 0x7F) rxCount++;
        if (readRxBuffer == 0x06) pendingACK = false;
        if (readRxBuffer == 0x15) txRESEND = true;
        break;
    case rx::Data1:
        rxData1 = readRxBuffer;
        rxCount++;
        break;
    case rx::Data2:
        rxData2 = readRxBuffer;
        rxCount++;
        break;
    case rx::Data3:
        rxData3 = readRxBuffer;
        rxCount++;
        break;
    case rx::Data4:
        rxData4 = readRxBuffer;
        rxCount++;
        break;
    case rx::Data5:
        rxData5 = readRxBuffer;
        rxCount++;
        break;
    case rx::End:
        if (readRxBuffer == 0xF7) { confirmedPayload = true; txACKNext = true;
        if (rxData1 == 'I' && rxData2 == 'R' && rxData3 == 'S' && rxData4 == 'E' && rxData5 == 'N') IR_SENSOR = true; }
        else txNAKNext = true;
        rxCount = rx::Idle;
        break;
    }
    sei();
}

void txPayload(unsigned char payload[], bool retry)
{
    if (retry) { // Allow only one retry then give up
        confirmedPayload = false;
        txRESEND         = false;
        if (lastTxPayload == payload) {
            pendingACK = false;
            return;
        }
    }
    for (uint8_t i = 0; i < 5; i++) lastTxPayload[i] = payload[i];
    loop_until_bit_is_set(UCSR1A, UDRE1);     // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0x7F;
    for (uint8_t i = 0; i < 5; i++) {
        loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
        if (!txRESEND) UDR1 = (0xFF & (int)payload[i]);
    }
    loop_until_bit_is_set(UCSR1A, UDRE1);     // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0xF7;
    pendingACK = true;
}

void txACK(bool ACK)
{
    confirmedPayload = false;
    if (ACK) {
        loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
        UDR1 = 0x06; // ACK HEX
        confirmedPayload = false;
        txACKNext = false;
    } else {
        loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
        UDR1 = 0x15; // NACK HEX
        txNAKNext = false;
    }
}
