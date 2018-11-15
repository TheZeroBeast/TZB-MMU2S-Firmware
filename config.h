// config.h - main configuration file


#define FW_VERSION 213 // example: 103 means version 1.0.3
#define FW_BUILDNR 999 // number of commits in 'master'

// timer0
//#define TIMER0_EVERY_1ms    _every_1ms    //1ms callback
//#define TIMER0_EVERY_10ms   _every_10ms   //10ms callback
//#define TIMER0_EVERY_100ms  _every_100ms  //100ms callback

//#define green_board

// shr16 - 16bit shift register
// pinout - hardcoded
//#define SHR16_CLK //signal d13 - PC7
//#define SHR16_LAT //signal d10 - PB6
//#define SHR16_DAT //signal d9  - PB5

// shift register outputs
// LEDS - hardcoded
//#define SHR16_LEDG0          0x0100
//#define SHR16_LEDR0          0x0200
//#define SHR16_LEDG1          0x0400
//#define SHR16_LEDR1          0x0800
//#define SHR16_LEDG2          0x1000
//#define SHR16_LEDR2          0x2000
//#define SHR16_LEDG3          0x4000
//#define SHR16_LEDR3          0x8000
//#define SHR16_LEDG4          0x0040
//#define SHR16_LEDR4          0x0080
#define SHR16_LED_MSK 0xffc0

// TMC2130 Direction/Enable signals - hardcoded
//#define SHR16_DIR_0          0x0001
//#define SHR16_ENA_0          0x0002
//#define SHR16_DIR_1          0x0004
//#define SHR16_ENA_1          0x0008
//#define SHR16_DIR_2          0x0010
//#define SHR16_ENA_2          0x0020
#define SHR16_DIR_MSK 0x0015
#define SHR16_ENA_MSK 0x002c

// UART0 (USB)
#define UART0_BDR 115200

// UART1 (fullduplex 5V UART to master, i.e. Prusa i3, MK3)
#define UART1_BDR 115200

// stdin & stdout uart0/1
#define UART_STD 0
// communication uart0/1
#define UART_COM 1

// TMC2130 - Trinamic stepper driver
// pinout - hardcoded
// spi:
#define TMC2130_SPI_RATE 0 // fosc/4 = 4MHz
#define TMC2130_SPCR SPI_SPCR(TMC2130_SPI_RATE, 1, 1, 1, 0)
#define TMC2130_SPSR SPI_SPSR(TMC2130_SPI_RATE)


// params:
// SG_THR stallguard treshold (sensitivity), range -64..63
// the stall guard value represents the load angle. if it reaches 0,
// the motor stalls. It's a 10 bit value, with  1023 in idle load (theoretically)
// According to the whole setup, that treshold should be tuned for accurate
// stall detection.
// Tuning: increase treshold, if stall detection triggers at normal loads
//   decrese treshold, if stall detection triggers too late
#define TMC2130_SG_THR_PUL 5
#define TMC2130_SG_THR_SEL 6 // 20 didn't work, 15 did, decreased further to 12
#define TMC2130_SG_THR_IDL 6 // optimized value: 4 (8 and 6 didn't work)


// TCOOLTHRS coolstep treshold, usable range 400-600, unit is 1/13MHz ~= 75ns
// below that equivalent speed the stall detection is disabled
#define TMC2130_TCOOLTHRS_AX_PUL 450 // 450 = 33.8us which is equal to a speed of 115 full steps/s
#define TMC2130_TCOOLTHRS_AX_SEL 400
#define TMC2130_TCOOLTHRS_AX_IDL 400 // 4100 1.0e9 ns/second / (200 uSteps/s * 256 uSteps/fullstep / 16 uStesp/fullstep)/75

// currents for pulley, selector and idler
#define CURRENT_HOLDING_STEALTH     { 1,  7, 16}
#define CURRENT_HOLDING_NORMAL      { 1, 10, 22}
#define CURRENT_RUNNING_STEALTH     {30, 35, 35}
#define CURRENT_RUNNING_NORMAL      {30, 35, 35}
#define CURRENT_HOMING              { 1, 35, 28}

// speeds and accelerations
#define MAX_SPEED_PUL 2000 // micro steps
#define MAX_SPEED_SEL 8000 // micro steps
#define MAX_SPEED_STEALTH_SEL 3000 // micro steps
#define MAX_SPEED_IDL 3000 // micro steps   changed from 1800 to 3000 11 Nov 18 - testing
#define ACC_NORMAL 80000 // micro steps / s²
#define ACC_STEALTH 15000 // micro steps / s²
#define ACC_FEED_NORMAL 1400 // micro steps / s²
#define ACC_FEED_STEALTH 1000 // micro steps / s²
#define ACC_IDL_NORMAL 20000 // micro steps / s² changed from 25k to 30k 11 Nov 18 - testing

//mode
#define HOMING_MODE 0
#define NORMAL_MODE 1
#define STEALTH_MODE 2

//ADC configuration
#define ADC_CHAN_MSK      0b0000000000100000 //used AD channels bit mask (ADC5)
#define ADC_CHAN_CNT      1          //number of used channels)
#define ADC_OVRSAMPL      1          //oversampling multiplier
#define ADC_READY         _adc_ready //ready callback


#define AX_PUL 0 // Pulley (Filament Drive)
#define AX_SEL 1 // Selector
#define AX_IDL 2 // Idler

#define PIN_STP_IDL_HIGH (PORTD |= 0x40)
#define PIN_STP_IDL_LOW (PORTD &= ~0x40)
#define PIN_STP_SEL_HIGH (PORTD |= 0x10)
#define PIN_STP_SEL_LOW (PORTD &= ~0x10)
#define PIN_STP_PUL_HIGH (PORTB |= 0x10)
#define PIN_STP_PUL_LOW (PORTB &= ~0x10)

#define TOOLSYNC 20                         // number of tool change (T) commands before a selector resync is performed


// signals (from interrupts to main loop)
#define SIG_ID_BTN 1 // any button changed

// states (<0 =error)
#define STA_INIT 0  // setup - initialization
#define STA_BUSY 1  // working
#define STA_READY 2 // ready - accepting commands

#define STA_ERR_TMC0_SPI -1     // TMC2130 axis0 spi error - not responding
#define STA_ERR_TMC0_MSC -2     // TMC2130 axis0 motor error - short circuit
#define STA_ERR_TMC0_MOC -3     // TMC2130 axis0 motor error - open circuit
#define STA_ERR_TMC0_PIN_STP -4 // TMC2130 axis0 pin wirring error - stp signal
#define STA_ERR_TMC0_PIN_DIR -5 // TMC2130 axis0 pin wirring error - dir signal
#define STA_ERR_TMC0_PIN_ENA -6 // TMC2130 axis0 pin wirring error - ena signal

#define STA_ERR_TMC1_SPI -11     // TMC2130 axis1 spi error - not responding
#define STA_ERR_TMC1_MSC -12     // TMC2130 axis1 motor error - short circuit
#define STA_ERR_TMC1_MOC -13     // TMC2130 axis1 motor error - open circuit
#define STA_ERR_TMC1_PIN_STP -14 // TMC2130 axis1 pin wirring error - stp signal
#define STA_ERR_TMC1_PIN_DIR -15 // TMC2130 axis1 pin wirring error - dir signal
#define STA_ERR_TMC1_PIN_ENA -16 // TMC2130 axis1 pin wirring error - ena signal

#define STA_ERR_TMC2_SPI -21     // TMC2130 axis2 spi error - not responding
#define STA_ERR_TMC2_MSC -22     // TMC2130 axis2 motor error - short circuit
#define STA_ERR_TMC2_MOC -23     // TMC2130 axis2 motor error - open circuit
#define STA_ERR_TMC2_PIN_STP -24 // TMC2130 axis2 pin wirring error - stp signal
#define STA_ERR_TMC2_PIN_DIR -25 // TMC2130 axis2 pin wirring error - dir signal
#define STA_ERR_TMC2_PIN_ENA -26 // TMC2130 axis2 pin wirring error - ena signal

// number of extruders
#define EXTRUDERS 5


// diagnostic functions
#define _DIAG

// testing motion controller
//#define TESTING_STEALTH
