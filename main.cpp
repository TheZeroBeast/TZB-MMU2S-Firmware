//! @file

#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include "shr16.h"
#include "adc.h"
#include "uart.h"
#include "spi.h"
#include "tmc2130.h"
#include "abtn3.h"
#include "mmctl.h"
#include "motion.h"
#include "Buttons.h"
#include <avr/wdt.h>
#include "permanent_storage.h"


// public variables:
int8_t sys_state = 0;
uint8_t sys_signals = 0;
bool fsensor_triggered = false;
uint8_t tmc2130_mode = NORMAL_MODE; // STEALTH_MODE;

#if (UART_COM == 0)
FILE *uart_com = uart0io;
#elif (UART_COM == 1)
FILE *uart_com = uart1io;
#endif //(UART_COM == 0)

extern "C" void process_commands(FILE *inout);

//! @brief Initialization after reset
//!
//! button | action
//! ------ | ------
//! middle | enter setup
//! right  | continue after error
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 00 | 00 | 00 | 00 | 0b | Shift register initialized
//! 00 | 00 | 00 | 0b | 00 | uart initialized
//! 00 | 00 | 0b | 00 | 00 | spi initialized
//! 00 | 0b | 00 | 00 | 00 | tmc2130 initialized
//! 0b | 00 | 00 | 00 | 00 | A/D converter initialized
//! b0 | b0 | b0 | b0 | b0 | Error, filament detected, still present
//! 0b | 0b | 0b | 0b | 0b | Error, filament detected, no longer present, continue by right button click
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
void setup()
{

    shr16_init(); // shift register
    led_blink(0);
    delay(1000);  // wait for boot ok printer

    uart0_init(); //uart0
    uart1_init(); //uart1
    led_blink(1);


#if (UART_STD == 0)
    stdin = uart0io;  // stdin = uart0
    stdout = uart0io; // stdout = uart0
#elif(UART_STD == 1)
    stdin = uart1io;  // stdin = uart1
    stdout = uart1io; // stdout = uart1
#endif //(UART_STD == 1)


    bool requestMenu = false;
    fprintf_P(uart_com, PSTR("start\n")); //startup message

    spi_init();
    led_blink(2);

    tmc2130_init(HOMING_MODE); // trinamic, homing
    led_blink(3);


    adc_init(); // ADC
    led_blink(4);

    init_Pulley();


    if (buttonClicked() == Btn::middle) {
        requestMenu = true;
    }

    // if FINDA is sensing filament do not home
    while (digitalRead(A1) == 1) {
        while (Btn::right != buttonClicked()) {
            if (digitalRead(A1) == 1) {
                shr16_set_led(0x2aa);
            } else {
                shr16_set_led(0x155);
            }
            delay(300);
            shr16_set_led(0x000);
            delay(300);
        }
    }

    home();
    // TODO 2: add reading previously stored mode (stealth/normal) from eeprom

    tmc2130_init(tmc2130_mode); // trinamic, initialize all axes


    // check if to goto the settings menu
    if (requestMenu) {
        setupMenu();
    }
}

//! @brief Select filament menu
//!
//! Select filament by pushing left and right button, park position can be also selected.
//!
//! button | action
//! ------ | ------
//! left   | select previous filament
//! right  | select next filament
//!
//! LED indication of states
//!
//! RG | RG | RG | RG | RG | meaning
//! -- | -- | -- | -- | -- | ------------------------
//! 01 | 00 | 00 | 00 | 00 | filament 1
//! 00 | 01 | 00 | 00 | 00 | filament 2
//! 00 | 00 | 01 | 00 | 00 | filament 3
//! 00 | 00 | 00 | 01 | 00 | filament 4
//! 00 | 00 | 00 | 00 | 01 | filament 5
//! 00 | 00 | 00 | 00 | bb | park position
//!
//! @n R - Red LED
//! @n G - Green LED
//! @n 1 - active
//! @n 0 - inactive
//! @n b - blinking
void manual_extruder_selector()
{
    shr16_set_led(1 << 2 * (4 - active_extruder));

#ifdef TESTING_STEALTH
    if (buttonClicked() != Btn::none) {
        switch (buttonClicked()) {
        case Btn::right:
            if (active_extruder < EXTRUDERS) {
                select_extruder(active_extruder + 1);
            }
            break;
        case Btn::left:
            if (active_extruder > 0) {
                select_extruder(active_extruder - 1);
            }
            break;
        default:
            break;
        }
    }
#else
    if ((Btn::left | Btn::right) & buttonClicked()) {
        switch (buttonClicked()) {
        case Btn::right:
            if (active_extruder < EXTRUDERS) {
                select_extruder(active_extruder + 1);
            }
            break;
        case Btn::left:
            if (active_extruder > 0) {
                select_extruder(active_extruder - 1);
            }
            break;
        default:
            break;
        }
    }
#endif

    if (active_extruder == 5) {
        shr16_set_led(2 << 2 * 0);
        delay(50);
        shr16_set_led(1 << 2 * 0);
        delay(50);
    }
}


//! @brief main loop
//!
//! It is possible to manually select filament and feed it when not printing.
//!
//! button | action
//! ------ | ------
//! middle | feed filament
//!
//! @copydoc manual_extruder_selector()
void loop()
{
    process_commands(uart_com);
    
    if (!isPrinting) {
        manual_extruder_selector();
#ifndef TESTING_STEALTH
        if (Btn::middle == buttonClicked() && active_extruder < 5) {
            shr16_set_led(2 << 2 * (4 - active_extruder));
            if (Btn::middle == buttonClicked()) {
                feed_filament();
            }
        }
    }
#endif
}

extern "C" {
    void process_commands(FILE *inout)
    {
        static char line[32];
        static int count = 0;
        int c = -1;
        if (count < 32) {
            if ((c = getc(inout)) >= 0) {
                if (c == '\r') {
                    c = 0;
                }
                if (c == '\n') {
                    c = 0;
                }
                line[count++] = c;
            }
        } else {
            count = 0;
            //overflow
        }
        int value = 0;
        int value0 = 0;

        if ((count > 0) && (c == 0)) {
            //line received
            //printf_P(PSTR("line received: '%s' %d\n"), line, count);
            count = 0;
            if (sscanf_P(line, PSTR("T%d"), &value) > 0) {
                //T-code scanned
                if ((value >= 0) && (value < EXTRUDERS)) {
                    fprintf_P(inout, PSTR("ok\n"));
                    toolChange(value);
                }
            } else if (sscanf_P(line, PSTR("L%d"), &value) > 0) {
                // Load filament
                if ((value >= 0) && (value < EXTRUDERS) && !isFilamentLoaded) {

                    select_extruder(value);
                    delay(10);
                    feed_filament();
                    delay(100);
                    fprintf_P(inout, PSTR("ok\n"));
                }
            } else if (sscanf_P(line, PSTR("M%d"), &value) > 0) {
                // M0: set to normal mode; M1: set to stealth mode
                switch (value) {
                case 0:
                    tmc2130_mode = NORMAL_MODE;
                    break;
                case 1:
                    tmc2130_mode = STEALTH_MODE;
                    break;
                default:
                    return;
                }
                //init all axes
                tmc2130_init(tmc2130_mode);
                fprintf_P(inout, PSTR("ok\n"));
            } else if (sscanf_P(line, PSTR("U%d"), &value) > 0) {
                // Unload filament
                unload_filament_withSensor();
                delay(200);
                fprintf_P(inout, PSTR("ok\n"));
                isPrinting = false;
                trackToolChanges = 0;
            } else if (sscanf_P(line, PSTR("X%d"), &value) > 0) {
                if (value == 0) { // MMU reset
                    wdt_enable(WDTO_15MS);
                }
            } else if (sscanf_P(line, PSTR("P%d"), &value) > 0) {
                if (value == 0) { // Read finda
                    fprintf_P(inout, PSTR("%dok\n"), digitalRead(A1));
                }
            } else if (sscanf_P(line, PSTR("S%d"), &value) > 0) {
                if (value == 0) { // return ok
                    fprintf_P(inout, PSTR("ok\n"));
                } else if (value == 1) { // Read version
                    fprintf_P(inout, PSTR("%dok\n"), FW_VERSION);
                } else if (value == 2) { // Read build nr
                    fprintf_P(inout, PSTR("%dok\n"), FW_BUILDNR);
                }
            } else if (sscanf_P(line, PSTR("F%d %d"), &value, &value0) > 0) {
                if (((value >= 0) && (value < EXTRUDERS)) && ((value0 >= 0) && (value0 <= 2))) {
                    filament_type[value] = value0;
                    fprintf_P(inout, PSTR("ok\n"));
                }
            } else if (sscanf_P(line, PSTR("C%d"), &value) > 0) {
                if (value == 0) // C0 continue loading current filament (used after T-code), maybe add different code for
                    // each extruder (the same way as T-codes) in the future?
                {
                    load_filament_into_extruder();
                    fprintf_P(inout, PSTR("ok\n"));
                }
                if (value == 1) {  // used if finda doesn't see filament, attempt to cut and advise print to try again
                    //fprintf_P(inout, PSTR("ok\n"));
                    fprintf_P(inout, PSTR("not_ok\n"));
                }
            } else if (sscanf_P(line, PSTR("E%d"), &value) > 0) {
                if ((value >= 0) && (value < EXTRUDERS)) { // Ex: eject filament
                    eject_filament(value);
                    fprintf_P(inout, PSTR("ok\n"));
                }
            } else if (sscanf_P(line, PSTR("R%d"), &value) > 0) {
                if (value == 0) { // R0: recover after eject filament
                    recover_after_eject();
                    fprintf_P(inout, PSTR("ok\n"));
                }
            } else if (sscanf_P(line, PSTR("FS%d"), &value) > 0) {
                if (value == 0) { // FS1: MK3 fsensor triggered
                    fprintf_P(inout, PSTR("ok\n"));
                } else if (value == 1) {
                    fsensor_triggered = true;
                    fprintf_P(inout, PSTR("ok\n"));
                }
            } else {
                // nothing received
            }
        }
    }
} // extern C

void process_signals()
{
    // what to do here?
}

void fault_handler(Fault id)
{
    while (1) {
        shr16_set_led(id + 1);
        delay(1000);
        shr16_set_led(0);
        delay(2000);
    }
}

//****************************************************************************************************
//* this routine is the common routine called for fixing the filament issues (loading or unloading)
//****************************************************************************************************
void fixTheProblem(void) {

  engage_filament_pulley(false);                     // park the idler stepper motor
  tmc2130_disable_axis(AX_SEL, tmc2130_mode);
  tmc2130_disable_axis(AX_IDL, tmc2130_mode);

  while (1) {
      //  wait until key is entered to proceed  (this is to allow for operator intervention)
      if (Btn::middle == buttonClicked()) {
          break;
      }
      shr16_set_led(2 << 2 * (4 - active_extruder));
      delay(100);
      shr16_set_led(0x000);
      delay(10);
      process_commands(uart_com);
  }

  tmc2130_init_axis(AX_SEL, tmc2130_mode);           // turn ON the selector stepper motor
  tmc2130_init_axis(AX_IDL, tmc2130_mode);           // turn ON the idler stepper motor

  while (homeSelectorSmooth());
  reset_positions(AX_SEL, 0, active_extruder, ACC_NORMAL);
  while (homeIdlerSmooth());
  //delay(50);  
  reset_positions(AX_IDL, 0, active_extruder, ACC_NORMAL);
  isFilamentLoaded = false;
  delay(1);                                          // wait for 1 millisecond
}

bool load_filament_withSensor()
{

    engage_filament_pulley(true); // if idler is in parked position un-park him get in contact with filament
    tmc2130_init_axis(AX_PUL, tmc2130_mode);
    uint8_t current_loading_normal[3] = CURRENT_LOADING_NORMAL;
    uint8_t current_loading_stealth[3] = CURRENT_LOADING_STEALTH;
    uint8_t current_running_normal[3] = CURRENT_RUNNING_NORMAL;
    uint8_t current_running_stealth[3] = CURRENT_RUNNING_STEALTH;
    uint8_t current_holding_normal[3] = CURRENT_HOLDING_NORMAL;
    uint8_t current_holding_stealth[3] = CURRENT_HOLDING_STEALTH; 
    const uint16_t stepsToExtruder = 8000; // BowdenLength::get();
    unsigned long startTime, currentTime;
    int flag;

    // load filament until FINDA senses end of the filament, means correctly loaded into the selector
    // we can expect something like 570 steps to get in sensor
 
    if (tmc2130_mode == NORMAL_MODE) {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                         current_loading_normal[AX_PUL]);
    } else {
        tmc2130_init_axis_current_normal(AX_PUL, current_holding_stealth[AX_PUL],
                                         current_loading_stealth[AX_PUL]);
    }
    // RMM:TODO Handle different modes
    if (moveSmooth(AX_PUL, 2000, 650, false, false, ACC_NORMAL, true) == MR_SuccesstoFinda) {
        if (tmc2130_mode == NORMAL_MODE) {
            tmc2130_init_axis_current_normal(AX_PUL, current_holding_normal[AX_PUL],
                                             current_running_normal[AX_PUL]);
        } else {
            tmc2130_init_axis_current_normal(AX_PUL, current_holding_stealth[AX_PUL],
                                             current_running_stealth[AX_PUL]);
        }
        moveSmooth(AX_PUL, stepsToExtruder, MAX_SPEED_PUL, false, false, ACC_FEED_NORMAL);

        process_commands(uart_com);

        startTime = millis();
        flag = 0;
        
        while (flag == 0) {
            currentTime = millis();
            if ((currentTime - startTime) > 8000) {
              fixTheProblem();
              load_filament_withSensor();
              startTime = millis();
            }

            move_pulley(1,MAX_SPEED_PUL);
            process_commands(uart_com);
            if (fsensor_triggered) {
              flag = 1;
              fsensor_triggered = false;
            }
            delayMicroseconds(600);
        }

        moveSmooth(AX_PUL, 350, 350,false);
        isFilamentLoaded = true;  // filament loaded
        return true;
    } fixTheProblem();
    load_filament_withSensor();
    return false;
}
