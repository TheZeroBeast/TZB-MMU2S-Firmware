// config.h - main configuration file


#define FW_VERSION  215 // example: 103 means version 1.0.3
#define FW_BUILDNR  263 // number of commits in 'master'

#define WAKE_TIMER            900000        //15m
#define TXTimeout        (uint8_t)60        //60ms

//#define green_board

// shr16 - 16bit shift register
// pinout - hardcoded
//#define SHR16_CLK //signal d13 - PC7
//#define SHR16_LAT //signal d10 - PB6
//#define SHR16_DAT //signal d9  - PB5

// shift register outputs
// LEDS - hardcoded
#define SHR16_LEDG0           0x0100
#define SHR16_LEDR0           0x0200
#define SHR16_LEDG1           0x0400
#define SHR16_LEDR1           0x0800
#define SHR16_LEDG2           0x1000
#define SHR16_LEDR2           0x2000
#define SHR16_LEDG3           0x4000
#define SHR16_LEDR3           0x8000
#define SHR16_LEDG4           0x0040
#define SHR16_LEDR4           0x0080
#define SHR16_LED_MSK         0xffc0
// TMC2130 Direction/Enable signals - hardcoded
#define SHR16_DIR_PUL          0x0001
#define SHR16_ENA_PUL          0x0002
#define SHR16_DIR_SEL          0x0004
#define SHR16_ENA_SEL          0x0008
#define SHR16_DIR_IDL          0x0010
#define SHR16_ENA_IDL          0x0020
#define SHR16_DIR_MSK        (SHR16_DIR_PUL + SHR16_DIR_SEL + SHR16_DIR_IDL)
#define SHR16_ENA_MSK        (SHR16_ENA_PUL + SHR16_ENA_SEL + SHR16_ENA_IDL)

// UART0 (USB)
#define UART0_BDR 115200

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
#define TMC2130_SG_THR_SEL 6
#define TMC2130_SG_THR_IDL 6
#define TMC2130_SG_THR_HOM_IDL 4

// TCOOLTHRS coolstep treshold, usable range 400-600, unit is 1/13MHz ~= 75ns
// below that equivalent speed the stall detection is disabled
#define TMC2130_TCOOLTHRS_AX_PUL 450 // 450 = 33.8us which is equal to a speed of 115 full steps/s
#define TMC2130_TCOOLTHRS_AX_SEL 400
#define TMC2130_TCOOLTHRS_AX_IDL 400 // 400 1.0e9 ns/second / (200 uSteps/s * 256 uSteps/fullstep / 16 uStesp/fullstep)/75

// currents for pulley, selector and idler
#define CURRENT_HOLDING_STEALTH     { 1,  7, 16}
#define CURRENT_HOLDING_NORMAL      { 1, 10, 22}
#define CURRENT_RUNNING_STEALTH     {30, 35, 35}
#define CURRENT_RUNNING_NORMAL      {30, 35, 35}
#define CURRENT_HOMING              { 1, 35, 50}

// speeds and accelerations
#define MAX_SPEED_SEL 5000 // micro steps
#define MAX_SPEED_STEALTH_SEL 3000 // micro steps
#define MAX_SPEED_IDL 3000 // micro steps
#define ACC_NORMAL 80000 // micro steps / s²
#define ACC_STEALTH 15000 // micro steps / s²
#define ACC_IDL_NORMAL 25000 // micro steps / s²

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

#define TOOLSYNC 100                         // number of tool change (T) commands before a selector resync is performed

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

// Type Definitions
// filament types (0: default; 1:flex; 2: PVA)
// Default
#define TYPE_0_MAX_SPPED_PUL                  4000  //  S/S
#define TYPE_0_ACC_FEED_PUL                   3000  //  S/S/S
#define TYPE_0_STEPS_MK3FSensor_To_Bondtech    270  //  STEPS
#define TYPE_0_FILAMENT_PARKING_STEPS         -610  //  STEPS
#define TYPE_0_FSensor_Sense_STEPS            1200  //  STEPS
#define TYPE_0_FEED_SPEED_PUL                  600  //  S/S
#define TYPE_0_L2ExStageOne                    350  //  S/S
#define TYPE_0_L2ExStageTwo                    390  //  S/S
// Flex
#define TYPE_1_MAX_SPPED_PUL                   400  //  S/S
#define TYPE_1_ACC_FEED_PUL                    100  //  S/S/S
#define TYPE_1_STEPS_MK3FSensor_To_Bondtech    330  //  STEPS
#define TYPE_1_FILAMENT_PARKING_STEPS         -610  //  STEPS
#define TYPE_1_FSensor_Sense_STEPS            1300  //  STEPS
#define TYPE_1_FEED_SPEED_PUL                  300  //  S/S
#define TYPE_1_L2ExStageOne                    200  //  S/S
#define TYPE_1_L2ExStageTwo                    200  //  S/S
// PVA
#define TYPE_2_MAX_SPPED_PUL                  2800  //  S/S
#define TYPE_2_ACC_FEED_PUL                   1500  //  S/S/S
#define TYPE_2_STEPS_MK3FSensor_To_Bondtech    300  //  STEPS
#define TYPE_2_FILAMENT_PARKING_STEPS         -610  //  STEPS
#define TYPE_2_FSensor_Sense_STEPS            1200  //  STEPS
#define TYPE_2_FEED_SPEED_PUL                  550  //  S/S
#define TYPE_2_L2ExStageOne                    350  //  S/S
#define TYPE_2_L2ExStageTwo                    390  //  S/S

// number of extruders
#define EXTRUDERS 5

// CONFIG
//#define _CONFIG_H
