//tmc2130.c - Trinamic stepper driver

#include "tmc2130.h"
#include <avr/io.h>
#include "spi.h"
#include <stdio.h>
#include <avr/pgmspace.h>

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
uint8_t tmc2130_usteps2mres(uint16_t usteps);

int8_t tmc2130_wr_CHOPCONF(uint8_t axis, uint8_t toff, uint8_t hstrt, uint8_t hend, uint8_t fd3, uint8_t disfdcc, uint8_t rndtf, uint8_t chm, uint8_t tbl, uint8_t vsense, uint8_t vhighfs, uint8_t vhighchm, uint8_t sync, uint8_t mres, uint8_t intpol, uint8_t dedge, uint8_t diss2g)
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
	//uint32_t valr = 0;
	//tmc2130_rd(axis, TMC2130_REG_CHOPCONF, &valr);
	//printf_P(PSTR("tmc2130_wr_CHOPCONF out=0x%08lx in=0x%08lx\n"), val, valr);
	//return (val == valr)?0:-1;
	return 0;
}

void tmc2130_wr_PWMCONF(uint8_t axis, uint8_t pwm_ampl, uint8_t pwm_grad, uint8_t pwm_freq, uint8_t pwm_auto, uint8_t pwm_symm, uint8_t freewheel)
{
	uint32_t val = 0;
	val |= (uint32_t)(pwm_ampl & 255);
	val |= (uint32_t)(pwm_grad & 255) << 8;
	val |= (uint32_t)(pwm_freq & 3) << 16;
	val |= (uint32_t)(pwm_auto & 1) << 18;
	val |= (uint32_t)(pwm_symm & 1) << 19;
	val |= (uint32_t)(freewheel & 3) << 20;
	tmc2130_wr(axis, TMC2130_REG_PWMCONF, val);
}

void tmc2130_wr_TPWMTHRS(uint8_t axis, uint32_t val32)
{
	tmc2130_wr(axis, TMC2130_REG_TPWMTHRS, val32);
}


int8_t tmc2130_setup_chopper(uint8_t axis, uint8_t mres, uint8_t current_h, uint8_t current_r)
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
		if (tmc2130_wr_CHOPCONF(axis, toff, hstrt, hend, fd3, 0, rndtf, chm, tbl, 1, 0, 0, 0, mres, intpol, 0, 0)) return -1;
		tmc2130_wr(axis, TMC2130_REG_IHOLD_IRUN, 0x000f0000 | ((current_r & 0x1f) << 8) | (current_h & 0x1f));
	}
	else
	{
		if (tmc2130_wr_CHOPCONF(axis, toff, hstrt, hend, fd3, 0, 0, 0, tbl, 0, 0, 0, 0, mres, intpol, 0, 0)) return -1;
		tmc2130_wr(axis, TMC2130_REG_IHOLD_IRUN, 0x000f0000 | (((current_r >> 1) & 0x1f) << 8) | ((current_h >> 1) & 0x1f));
	}
	return 0;
}

inline uint16_t __tcoolthrs(uint8_t axis)
{
	switch (axis)
	{
	case 0: return TMC2130_TCOOLTHRS_0;
	case 1: return TMC2130_TCOOLTHRS_1;
	case 2: return TMC2130_TCOOLTHRS_2;
	}
	return TMC2130_TCOOLTHRS;
}

inline int8_t __sg_thr(uint8_t axis)
{
	switch (axis)
	{
	case 0: return TMC2130_SG_THR_0;
	case 1: return TMC2130_SG_THR_1;
	case 2: return TMC2130_SG_THR_2;
	}
	return TMC2130_SG_THR;
}

inline int8_t __curh(uint8_t axis)
{
	switch (axis)
	{
	case 0: return 1;
	case 1: return 20;
	case 2: return 24;
	}
	return 16;
}

inline int8_t __curr(uint8_t axis)
{
	switch (axis)
	{
	case 0: return 30;   
	case 1: return 35;   
	case 2: return 35;   
	}
	return 16;
}

inline int8_t __currh(uint8_t axis)
{
	switch (axis)
	{
	case 0: return 1;   
	case 1: return 20;  
	case 2: return 30;   
	}
	return 16;
}

inline int8_t __res(uint8_t axis)
{
	switch (axis)
	{
	case 0: return tmc2130_usteps2mres((uint16_t)2);
	case 1: return tmc2130_usteps2mres((uint16_t)2);
	case 2: return tmc2130_usteps2mres((uint16_t)16);
	}
	return 16;
}

uint8_t tmc2130_usteps2mres(uint16_t usteps)
{
	uint8_t mres = 8; while (mres && (usteps >>= 1)) mres--;
	return mres;
}

//axis config
//byte 0 bit 0..3 - mres
//byte 0 bit 4..5 - reserved
//byte 0 bit 6..7 - mode - 00-constOff 01-spreadCycle 10-stealthChop
//byte 1 bit 0..5 - curh
//byte 2 bit 0..5 - curr
//byte 3 
int8_t tmc2130_init_axis(uint8_t axis, uint8_t homing)
{
/* //silent mode
	tmc2130_setup_chopper(axis, 7, 16, 16);
	tmc2130_wr(axis, TMC2130_REG_TPOWERDOWN, 0x00000000);
	tmc2130_wr(axis, TMC2130_REG_COOLCONF, (((uint32_t)TMC2130_SG_THR) << 16));
	tmc2130_wr(axis, TMC2130_REG_TCOOLTHRS, 0);
	tmc2130_wr(axis, TMC2130_REG_GCONF, 0x00000004);
	tmc2130_wr_PWMCONF(axis, 150, 2, 2, 1, 0, 0);
	tmc2130_wr_TPWMTHRS(axis, 0);
*/
	//uint8_t mres = 7;
	//uint16_t tcoolthrs = 450;
	if (homing)
	{
		if (tmc2130_setup_chopper(axis, (uint32_t)__res(axis), (uint32_t)__curh(axis), (uint32_t)__curr(axis))) return -1;
	}
	else
	{
		if (tmc2130_setup_chopper(axis, (uint32_t)__res(axis), (uint32_t)__curh(axis), (uint32_t)__currh(axis))) return -1;
	}

	
	tmc2130_wr(axis, TMC2130_REG_TPOWERDOWN, 0x00000000);
	tmc2130_wr(axis, TMC2130_REG_COOLCONF, (((uint32_t)__sg_thr(axis)) << 16));
	tmc2130_wr(axis, TMC2130_REG_TCOOLTHRS, __tcoolthrs(axis));
	tmc2130_wr(axis, TMC2130_REG_GCONF, 0x00003180);
	return 0;
}

int8_t tmc2130_init_axis_current(uint8_t axis, uint8_t current_h, uint8_t current_r )
{

	if (tmc2130_setup_chopper(axis, (uint32_t)__res(axis), current_h, current_r)) return -1;

	tmc2130_wr(axis, TMC2130_REG_TPOWERDOWN, 0x00000000);
	tmc2130_wr(axis, TMC2130_REG_COOLCONF, (((uint32_t)__sg_thr(axis)) << 16));
	tmc2130_wr(axis, TMC2130_REG_TCOOLTHRS, __tcoolthrs(axis));
	tmc2130_wr(axis, TMC2130_REG_GCONF, 0x00003180);
	return 0;
}

uint8_t tmc2130_check_axis(uint8_t axis)
{
	return 0x3f;
}



int8_t tmc2130_init(uint8_t homing)
{
	DDRC |= 0x40;
	DDRD |= 0x80;
	DDRB |= 0x80;


	PORTC |= 0x40;
	PORTD |= 0x80;
	PORTB |= 0x80;   

	DDRD |= 0x10;
	DDRB |= 0x10;
	DDRD |= 0x40;
	PORTD &= ~0x10;
	PORTB &= ~0x10;
	PORTD &= ~0x40;

	int8_t ret = 0;
	ret += tmc2130_init_axis(0, homing)?-1:0;
	ret += tmc2130_init_axis(1,homing)?-2:0;
	ret += tmc2130_init_axis(2,homing)?-4:0;

	return ret;
}


uint16_t tmc2130_read_sg(uint8_t axis)
{
	uint32_t val32 = 0;
	tmc2130_rd(axis, TMC2130_REG_DRV_STATUS, &val32);
	return (val32 & 0x3ff);
}


inline void tmc2130_cs_low(uint8_t axis)
{
	switch (axis)
	{
	case 0: PORTC &= ~0x40; break;
	case 1: PORTD &= ~0x80; break;
	case 2: PORTB &= ~0x80; break; //// black board
	}
}

inline void tmc2130_cs_high(uint8_t axis)
{
	switch (axis)
	{
	case 0: PORTC |= 0x40; break;
	case 1: PORTD |= 0x80; break;
	case 2: PORTB |= 0x80; break; //// black board
	}
}

//Arduino SPI
//#define TMC2130_SPI_ENTER()    SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE3))
//#define TMC2130_SPI_TXRX       SPI.transfer
//#define TMC2130_SPI_LEAVE      SPI.endTransaction

//spi
#define TMC2130_SPI_ENTER()    spi_setup(TMC2130_SPCR, TMC2130_SPSR)
#define TMC2130_SPI_TXRX       spi_txrx
#define TMC2130_SPI_LEAVE()

uint8_t tmc2130_tx(uint8_t axis, uint8_t addr, uint32_t wval)
{
	//datagram1 - request
	TMC2130_SPI_ENTER();
	tmc2130_cs_low(axis);
	TMC2130_SPI_TXRX(addr); // address
	TMC2130_SPI_TXRX((wval >> 24) & 0xff); // MSB
	TMC2130_SPI_TXRX((wval >> 16) & 0xff);
	TMC2130_SPI_TXRX((wval >> 8) & 0xff);
	TMC2130_SPI_TXRX(wval & 0xff); // LSB
	tmc2130_cs_high(axis);
	TMC2130_SPI_LEAVE();
}

uint8_t tmc2130_rx(uint8_t axis, uint8_t addr, uint32_t* rval)
{
	//datagram1 - request
	TMC2130_SPI_ENTER();
	tmc2130_cs_low(axis);
	TMC2130_SPI_TXRX(addr); // address
	TMC2130_SPI_TXRX(0); // MSB
	TMC2130_SPI_TXRX(0);
	TMC2130_SPI_TXRX(0);
	TMC2130_SPI_TXRX(0); // LSB
	tmc2130_cs_high(axis);
	TMC2130_SPI_LEAVE();
	//datagram2 - response
	TMC2130_SPI_ENTER();
	tmc2130_cs_low(axis);
	uint8_t stat = TMC2130_SPI_TXRX(0); // status
	uint32_t val32 = 0;
	val32 = TMC2130_SPI_TXRX(0); // MSB
	val32 = (val32 << 8) | TMC2130_SPI_TXRX(0);
	val32 = (val32 << 8) | TMC2130_SPI_TXRX(0);
	val32 = (val32 << 8) | TMC2130_SPI_TXRX(0); // LSB
	tmc2130_cs_high(axis);
	TMC2130_SPI_LEAVE();
	if (rval != 0) *rval = val32;
	return stat;
}

