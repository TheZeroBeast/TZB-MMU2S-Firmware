//tmc2130.cpp

#include "tmc2130.h"
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <SPI.h>


#define TMC2130_CS_0 //signal d5  - PC6
#define TMC2130_CS_1 //signal d6  - PD7
#define TMC2130_CS_2 //signal d7  - PE6

#define TMC2130_ST_0 //signal d4  - PD4
#define TMC2130_ST_1 //signal d8  - PB4
#define TMC2130_ST_2 //signal d12 - PD6

//TMC2130 registers
#define TMC2130_REG_GCONF      0x00 // 17 bits
#define TMC2130_REG_GSTAT      0x01 // 3 bits
#define TMC2130_REG_IOIN       0x04 // 8+8 bits
#define TMC2130_REG_IHOLD_IRUN 0x10 // 5+5+4 bits
#define TMC2130_REG_TPOWERDOWN 0x11 // 8 bits
#define TMC2130_REG_TSTEP      0x12 // 20 bits
#define TMC2130_REG_TPWMTHRS   0x13 // 20 bits
#define TMC2130_REG_TCOOLTHRS  0x14 // 20 bits
#define TMC2130_REG_THIGH      0x15 // 20 bits
#define TMC2130_REG_XDIRECT    0x2d // 32 bits
#define TMC2130_REG_VDCMIN     0x33 // 23 bits
#define TMC2130_REG_MSLUT0     0x60 // 32 bits
#define TMC2130_REG_MSLUT1     0x61 // 32 bits
#define TMC2130_REG_MSLUT2     0x62 // 32 bits
#define TMC2130_REG_MSLUT3     0x63 // 32 bits
#define TMC2130_REG_MSLUT4     0x64 // 32 bits
#define TMC2130_REG_MSLUT5     0x65 // 32 bits
#define TMC2130_REG_MSLUT6     0x66 // 32 bits
#define TMC2130_REG_MSLUT7     0x67 // 32 bits
#define TMC2130_REG_MSLUTSEL   0x68 // 32 bits
#define TMC2130_REG_MSLUTSTART 0x69 // 8+8 bits
#define TMC2130_REG_MSCNT      0x6a // 10 bits
#define TMC2130_REG_MSCURACT   0x6b // 9+9 bits
#define TMC2130_REG_CHOPCONF   0x6c // 32 bits
#define TMC2130_REG_COOLCONF   0x6d // 25 bits
#define TMC2130_REG_DCCTRL     0x6e // 24 bits
#define TMC2130_REG_DRV_STATUS 0x6f // 32 bits
#define TMC2130_REG_PWMCONF    0x70 // 22 bits
#define TMC2130_REG_PWM_SCALE  0x71 // 8 bits
#define TMC2130_REG_ENCM_CTRL  0x72 // 2 bits
#define TMC2130_REG_LOST_STEPS 0x73 // 20 bits


#define tmc2130_rd(axis, addr, rval) tmc2130_rx(axis, addr, rval)
#define tmc2130_wr(axis, addr, wval) tmc2130_tx(axis, addr | 0x80, wval)

uint8_t tmc2130_tx(uint8_t axis, uint8_t addr, uint32_t wval);
uint8_t tmc2130_rx(uint8_t axis, uint8_t addr, uint32_t* rval);

void tmc2130_wr_CHOPCONF(uint8_t axis, uint8_t toff, uint8_t hstrt, uint8_t hend, uint8_t fd3, uint8_t disfdcc, uint8_t rndtf, uint8_t chm, uint8_t tbl, uint8_t vsense, uint8_t vhighfs, uint8_t vhighchm, uint8_t sync, uint8_t mres, uint8_t intpol, uint8_t dedge, uint8_t diss2g)
{
	uint32_t val = 0;
	val |= (uint32_t)(toff & 15);
	val |= (uint32_t)(hstrt & 7) << 4;
	val |= (uint32_t)(hend & 15) << 7;
	val |= (uint32_t)(fd3 & 1) << 11;
	val |= (uint32_t)(disfdcc & 1) << 12;
	val |= (uint32_t)(rndtf & 1) << 13;
	val |= (uint32_t)(chm & 1) << 14;
	val |= (uint32_t)(tbl & 3) << 15;
	val |= (uint32_t)(vsense & 1) << 17;
	val |= (uint32_t)(vhighfs & 1) << 18;
	val |= (uint32_t)(vhighchm & 1) << 19;
	val |= (uint32_t)(sync & 15) << 20;
	val |= (uint32_t)(mres & 15) << 24;
	val |= (uint32_t)(intpol & 1) << 28;
	val |= (uint32_t)(dedge & 1) << 29;
	val |= (uint32_t)(diss2g & 1) << 30;
	tmc2130_wr(axis, TMC2130_REG_CHOPCONF, val);
}

void tmc2130_setup_chopper(uint8_t axis, uint8_t mres, uint8_t current_h, uint8_t current_r)
{
	uint8_t intpol = 1;
	uint8_t toff = 3; // toff = 3 (fchop = 27.778kHz)
	uint8_t hstrt = 5; //initial 4, modified to 5
	uint8_t hend = 1;
	uint8_t fd3 = 0;
	uint8_t rndtf = 0; //random off time
	uint8_t chm = 0; //spreadCycle
	uint8_t tbl = 2; //blanking time
	if (current_r <= 31)
	{
		tmc2130_wr_CHOPCONF(axis, toff, hstrt, hend, fd3, 0, rndtf, chm, tbl, 1, 0, 0, 0, mres, intpol, 0, 0);
		tmc2130_wr(axis, TMC2130_REG_IHOLD_IRUN, 0x000f0000 | ((current_r & 0x1f) << 8) | (current_h & 0x1f));
	}
	else
	{
		tmc2130_wr_CHOPCONF(axis, toff, hstrt, hend, fd3, 0, 0, 0, tbl, 0, 0, 0, 0, mres, intpol, 0, 0);
		tmc2130_wr(axis, TMC2130_REG_IHOLD_IRUN, 0x000f0000 | (((current_r >> 1) & 0x1f) << 8) | ((current_h >> 1) & 0x1f));
	}
}

int8_t tmc2130_init_axis(uint8_t axis)
{
	tmc2130_setup_chopper(axis, 7, 16, 16);
	tmc2130_wr(axis, TMC2130_REG_TPOWERDOWN, 0x00000000);
	tmc2130_wr(axis, TMC2130_REG_GCONF, 0x00000000);
}

void tmc2130_do_step(uint8_t axis)
{
	switch (axis)
	{
	case 0:
		PORTD |= 0x10;
		asm("nop");
		PORTD &= ~0x10;
		break;
	case 1:
		PORTB |= 0x10;
		asm("nop");
		PORTB &= ~0x10;
		break;
	case 2:
		PORTD |= 0x40;
		asm("nop");
		PORTD &= ~0x40;
		break;
	}
	asm("nop");
}

int8_t tmc2130_init()
{
	DDRC |= 0x40;
	DDRD |= 0x80;
	DDRE |= 0x40;
	PORTC |= 0x40;
	PORTD |= 0x80;
	PORTE |= 0x40;
	SPI.begin();

	DDRD |= 0x10;
	DDRB |= 0x10;
	DDRD |= 0x40;
	PORTD &= ~0x10;
	PORTB &= ~0x10;
	PORTD &= ~0x40;

	tmc2130_init_axis(0);
	tmc2130_init_axis(1);
	tmc2130_init_axis(2);

	uint32_t val32 = 0;
/*
	tmc2130_rd(0, TMC2130_REG_GSTAT, &val32);
	Serial.print("axis A 0x");
	Serial.println(val32, 16);

	tmc2130_rd(1, TMC2130_REG_GSTAT, &val32);
	Serial.print("axis B 0x");
	Serial.println(val32, 16);

	tmc2130_rd(2, TMC2130_REG_GSTAT, &val32);
	Serial.print("axis C 0x");
	Serial.println(val32, 16);
*/
	return 1;
}



inline void tmc2130_cs_low(uint8_t axis)
{
	switch (axis)
	{
	case 0: PORTC &= ~0x40; break;
	case 1: PORTD &= ~0x80; break;
	case 2: PORTE &= ~0x40; break;
	}
}

inline void tmc2130_cs_high(uint8_t axis)
{
	switch (axis)
	{
	case 0: PORTC |= 0x40; break;
	case 1: PORTD |= 0x80; break;
	case 2: PORTE |= 0x40; break;
	}
}

uint8_t tmc2130_tx(uint8_t axis, uint8_t addr, uint32_t wval)
{
	//datagram1 - request
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE3));
	tmc2130_cs_low(axis);
	SPI.transfer(addr); // address
	SPI.transfer((wval >> 24) & 0xff); // MSB
	SPI.transfer((wval >> 16) & 0xff);
	SPI.transfer((wval >> 8) & 0xff);
	SPI.transfer(wval & 0xff); // LSB
	tmc2130_cs_high(axis);
	SPI.endTransaction();
}

uint8_t tmc2130_rx(uint8_t axis, uint8_t addr, uint32_t* rval)
{
	//datagram1 - request
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE3));
	tmc2130_cs_low(axis);
	SPI.transfer(addr); // address
	SPI.transfer(0); // MSB
	SPI.transfer(0);
	SPI.transfer(0);
	SPI.transfer(0); // LSB
	tmc2130_cs_high(axis);
	SPI.endTransaction();
	//datagram2 - response
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE3));
	tmc2130_cs_low(axis);
	uint8_t stat = SPI.transfer(0); // status
	uint32_t val32 = 0;
	val32 = SPI.transfer(0); // MSB
	val32 = (val32 << 8) | SPI.transfer(0);
	val32 = (val32 << 8) | SPI.transfer(0);
	val32 = (val32 << 8) | SPI.transfer(0); // LSB
	tmc2130_cs_high(axis);
	SPI.endTransaction();
	if (rval != 0) *rval = val32;
	return stat;
}

