// spi.h - hardware SPI
#ifndef SPI_H
#define SPI_H

#include <inttypes.h>
#include <avr/io.h>
#include "config.h"

#define SPI_SPCR(rat, pha, pol, mst, dor)                                                                              \
    ((rat & 3) | (pha ? (1 << CPHA) : 0) | (pol ? (1 << CPOL) : 0) | (mst ? (1 << MSTR) : 0) | (dor ? (1 << DORD) : 0) \
     | (1 << SPE))
#define SPI_SPSR(rat) ((rat & 4) ? (1 << SPI2X) : 0)

// Bit numbers of data direction register:
#define DD_SCK 1
#define DD_MOSI 2
#define DD_MISO 3

inline void spi_init()
{
    DDRB &= ~((1 << DD_SCK) | (1 << DD_MOSI) | (1 << DD_MISO));
    DDRB |= (1 << DD_SCK) | (1 << DD_MOSI);
}

inline void spi_setup(uint8_t spcr, uint8_t spsr)
{
    SPCR = spcr;
    SPSR = spsr;
}

inline uint8_t spi_txrx(uint8_t tx)
{
    SPDR = tx;
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}

#endif // SPI_H
