// uart.cpp

#include "uart.h"
#include "Arduino.h"

bool rxFlag = false;

char rxBuf[BUFFER_SIZE];

ISR(USART1_RX_vect)
{
    cli();
    // Code to be executed when ISR fires
    //char ReceivedByte;
    //ReceivedByte = UDR1; // Fetch the received byte value into the variable "ByteReceived"
    
    volatile int rxn = 0;
    while ( !(UCSR1A & (1<<RXC1)) ) {
        if (rxn==BUFFER_SIZE) rxn=0;// if BUFFER_SIZE is reached, reset to start of buffer.
        rxBuf[rxn++] = UDR1;        // increment rxn and return new value.   
        rxFlag = true;                   // notify main of receipt of data.
    }
    sei();
}



/*
// public variables:
FILE _uart0io;
FILE _uart1io;



int uart0_putchar(char c, FILE *)
{
    Serial.write(c);
    return 0;
}
int uart0_getchar(FILE *)
{
    //  return 'A';
    return Serial.read();
}

int uart1_putchar(char c, FILE *)
{
    Serial1.write(c);
    return 0;
}
int uart1_getchar(FILE *)
{
    return Serial1.read();
}

void uart0_init(void)
{
    Serial.begin(UART0_BDR, SERIAL_8N2); //serial0 - USB
    fdev_setup_stream(uart0io, uart0_putchar, uart0_getchar,
                      _FDEV_SETUP_WRITE | _FDEV_SETUP_READ); //setup uart in/out stream
}

void uart1_init(void)
{
    Serial1.begin(UART1_BDR, SERIAL_8N2); //serial1
    fdev_setup_stream(uart1io, uart1_putchar, uart1_getchar,
                      _FDEV_SETUP_WRITE | _FDEV_SETUP_READ); //setup uart in/out stream
}*/
