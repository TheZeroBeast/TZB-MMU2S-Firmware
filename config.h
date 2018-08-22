//config.h - main configuration file

#define FW_VERSION 90 //it means version 0.9.0
#define FW_BUILDNR 72 //number of commits in 'master'

//timer0
//#define TIMER0_EVERY_1ms    _every_1ms    //1ms callback
//#define TIMER0_EVERY_10ms   _every_10ms   //10ms callback
//#define TIMER0_EVERY_100ms  _every_100ms  //100ms callback

//#define green_board

//shr16 - 16bit shift register
//pinout - hardcoded
//#define SHR16_CLK //signal d13 - PC7
//#define SHR16_LAT //signal d10 - PB6
//#define SHR16_DAT //signal d9  - PB5

//shift register outputs
//LEDS - hardcoded
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
#define SHR16_LED_MSK          0xffc0

//TMC2130 Direction/Enable signals - hardcoded
//#define SHR16_DIR_0          0x0001
//#define SHR16_ENA_0          0x0002
//#define SHR16_DIR_1          0x0004
//#define SHR16_ENA_1          0x0008
//#define SHR16_DIR_2          0x0010
//#define SHR16_ENA_2          0x0020
#define SHR16_DIR_MSK          0x0015
#define SHR16_ENA_MSK          0x002c

//UART0
#define UART0_BDR 115200

//UART1
#define UART1_BDR 115200

//stdin & stdout uart0/1
#define UART_STD 0
//communication uart0/1
#define UART_COM 1

//TMC2130 - Trinamic stepper driver
//pinout - hardcoded
//spi:
#define TMC2130_SPI_RATE       0 // fosc/4 = 4MHz
#define TMC2130_SPCR           SPI_SPCR(TMC2130_SPI_RATE, 1, 1, 1, 0)
#define TMC2130_SPSR           SPI_SPSR(TMC2130_SPI_RATE)
//params:
// SG_THR stallguard treshold (sensitivity), range -128..127, real 0-3
#define TMC2130_SG_THR_0       5
#define TMC2130_SG_THR_1       6
#define TMC2130_SG_THR_2       1
// TCOOLTHRS coolstep treshold, usable range 400-600
#define TMC2130_TCOOLTHRS_0    450
#define TMC2130_TCOOLTHRS_1    450
#define TMC2130_TCOOLTHRS_2    450

#define AX_IDL 2
#define AX_SEL 1
#define AX_PUL 0
//0 - IDLER
//1 - SELECTOR
//2 - PULLEY

//ADC configuration
#define ADC_CHAN_MSK      0b0000000000100000 //used AD channels bit mask (ADC5)
#define ADC_CHAN_CNT      1          //number of used channels)
#define ADC_OVRSAMPL      1          //oversampling multiplier
#define ADC_READY         _adc_ready //ready callback


//signals (from interrupts to main loop)
#define SIG_ID_BTN             1 // any button changed

//states (<0 =error)
#define STA_INIT               0 //setup - initialization
#define STA_BUSY               1 //working
#define STA_READY              2 //ready - accepting commands

#define STA_ERR_TMC0_SPI      -1 //TMC2130 axis0 spi error - not responding 
#define STA_ERR_TMC0_MSC      -2 //TMC2130 axis0 motor error - short circuit
#define STA_ERR_TMC0_MOC      -3 //TMC2130 axis0 motor error - open circuit
#define STA_ERR_TMC0_PIN_STP  -4 //TMC2130 axis0 pin wirring error - stp signal
#define STA_ERR_TMC0_PIN_DIR  -5 //TMC2130 axis0 pin wirring error - dir signal
#define STA_ERR_TMC0_PIN_ENA  -6 //TMC2130 axis0 pin wirring error - ena signal

#define STA_ERR_TMC1_SPI      -11 //TMC2130 axis1 spi error - not responding 
#define STA_ERR_TMC1_MSC      -12 //TMC2130 axis1 motor error - short circuit
#define STA_ERR_TMC1_MOC      -13 //TMC2130 axis1 motor error - open circuit
#define STA_ERR_TMC1_PIN_STP  -14 //TMC2130 axis1 pin wirring error - stp signal
#define STA_ERR_TMC1_PIN_DIR  -15 //TMC2130 axis1 pin wirring error - dir signal
#define STA_ERR_TMC1_PIN_ENA  -16 //TMC2130 axis1 pin wirring error - ena signal

#define STA_ERR_TMC2_SPI      -21 //TMC2130 axis2 spi error - not responding 
#define STA_ERR_TMC2_MSC      -22 //TMC2130 axis2 motor error - short circuit
#define STA_ERR_TMC2_MOC      -23 //TMC2130 axis2 motor error - open circuit
#define STA_ERR_TMC2_PIN_STP  -24 //TMC2130 axis2 pin wirring error - stp signal
#define STA_ERR_TMC2_PIN_DIR  -25 //TMC2130 axis2 pin wirring error - dir signal
#define STA_ERR_TMC2_PIN_ENA  -26 //TMC2130 axis2 pin wirring error - ena signal

//number of extruders
#define EXTRUDERS 5

//diagnostic functions
//#define _DIAG
