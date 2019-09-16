// uart.cpp
#include "uart.h"
#include "config.h"
#include "main.h"

volatile unsigned char readRxBuffer, rxData1 = 0, rxData2 = 0, rxData3 = 0,
                                     rxData4 = 0, rxData5 = 0;
volatile bool confirmedPayload = false, IR_SENSOR = false;
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
rx rxCount = rx::Idle;

inline rx& operator++(rx& byte, int)
{
    const int i = static_cast<int>(byte) + 1;
    byte = static_cast<rx>((i) % 7);
    return byte;
}

ISR(USART1_RX_vect)
{
    readRxBuffer = UDR1;
    switch (rxCount) {
    case rx::Idle:
        if (readRxBuffer == 0x7F) rxCount++;
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
        if (readRxBuffer == 0xF7) {
            if (rxData1 == 'I' && rxData2 == 'R' && rxData3 == 'S' && rxData4 == 'E' && rxData5 == 'N') IR_SENSOR = true;
            else confirmedPayload = true; 
        }
        rxCount = rx::Idle;
        break;
    }
}

void txPayload(unsigned char payload[])
{
    loop_until_bit_is_set(UCSR1A, UDRE1);     // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0x7F;
    for (uint8_t i = 0; i < 5; i++) {
        loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
        UDR1 = (0xFF & (int)payload[i]);
    }
    loop_until_bit_is_set(UCSR1A, UDRE1);     // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0xF7;
}

void txFINDAStatus(void)
{
    loop_until_bit_is_set(UCSR1A, UDRE1);     // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0x06;
    loop_until_bit_is_set(UCSR1A, UDRE1); // Do nothing until UDR is ready for more data to be written to it
    UDR1 = (uint8_t)isFilamentLoaded();
    loop_until_bit_is_set(UCSR1A, UDRE1);     // Do nothing until UDR is ready for more data to be written to it
    UDR1 = 0xF7;
}