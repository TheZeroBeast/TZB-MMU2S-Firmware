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
bool unloadatBoot = false;
bool mmuFSensorLoading = false;
bool m600RunoutChanging = false;
bool duplicateTCmd = false;
bool load_filament_at_toolChange = false;
//bool fixingTheProblems = false;
long startWakeTime, currentWakeTime;

uint8_t tmc2130_mode = NORMAL_MODE;

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
    permanentStorageInit();
    shr16_init(); // shift register
    led_blink(0);
    delay(1000);  // wait for boot ok printer
    startWakeTime = millis();
    led_blink(1);

    UCSR1A = (0 << U2X1); // baudrate multiplier
    UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (0 << UCSZ12);   // Turn on the transmission and reception circuitry
    UCSR1C = (0 << UMSEL11) | (0 << UMSEL10)| (0 << UPM11)
             | (0 << UPM10) | (1 << USBS1) | (1 << UCSZ11) | (1 << UCSZ10); // Use 8-bit character sizes
    UCSR1D = (0 << CTSEN) | (0 << RTSEN); // Disable flow control

    UBRR1H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
    UBRR1L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register

    UCSR1B |= (1 << RXCIE1);

    sei();

    txPayload("STR");

    bool requestMenu = false;
    if (buttonClicked() == Btn::middle) requestMenu = true;

    spi_init();
    led_blink(2);
    tmc2130_init(HOMING_MODE); // trinamic, homing
    led_blink(3);
    adc_init(); // ADC
    led_blink(4);

    shr16_clr_led();

    init_Pulley();
    homeIdlerSmooth(true);

    //add reading previously stored mode (stealth/normal) from eeprom
    tmc2130_init(tmc2130_mode); // trinamic, initialize all axes

    // check if to goto the settings menu
    if (requestMenu) {
        setupMenu();
    }

    if (digitalRead(A1)) isFilamentLoaded = true;
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
    shr16_clr_led();
    shr16_set_led(1 << 2 * (4 - active_extruder));

    if (((Btn::left | Btn::right) & buttonClicked()) && !digitalRead(A1)) {
        switch (buttonClicked()) {
        case Btn::right:
            if (active_extruder < EXTRUDERS) set_positions(active_extruder, active_extruder + 1, true);
            break;
        case Btn::left:
            if (active_extruder > 0) set_positions(active_extruder, active_extruder - 1, true);
            break;
        default:
            break;
        }
    }

    if (active_extruder == 5) {
        shr16_clr_led();
        shr16_set_led(3 << 2 * 0);
        delay(100);
        shr16_clr_led();
        delay(100);
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
    process_commands();

    if (!isPrinting) {
        manual_extruder_selector();
        if (Btn::middle == buttonClicked()) {
            if (active_extruder < EXTRUDERS) {
                shr16_clr_led();
                shr16_set_led(2 << 2 * (4 - active_extruder));
                if (Btn::middle == buttonClicked()) {
                    feed_filament();
                }
            } else if (active_extruder == EXTRUDERS) setupMenu();
        }
    }
    currentWakeTime = millis();
    if (((currentWakeTime - startWakeTime) > WAKE_TIMER) && !isFilamentLoaded) disableAllSteppers();
}

void process_commands()
{
    unsigned char tData1 = rxData1;                  // Copy volitale vars as local
    unsigned char tData2 = rxData2;
    unsigned char tData3 = rxData3;
    unsigned char tCSUM1 = rxCSUM1;
    unsigned char tCSUM2 = rxCSUM2;
    int16_t  tCSUM = ((tCSUM1 << 8) | tCSUM2);
    bool     confPayload = confirmedPayload;
    if (txRESEND) {
        txRESEND         = false;
        confirmedPayload = false;
        startRxFlag      = false;
        txPayload(lastTxPayload);
        return;
    }
    if ((confPayload && !(tCSUM == (tData1 + tData2 + tData3))) || txNAKNext) { // If confirmed with bad CSUM or NACK return has been requested
        txACK(false); // Send NACK Byte
    } else if (confPayload) {
        txACK();      // Send  ACK Byte


        if (tData1 == 'T') {
            //Tx Tool Change CMD Received
            if ((tData2 >= 0) && (tData2 < EXTRUDERS)) {
                if ((active_extruder == tData2) && isFilamentLoaded && !m600RunoutChanging) {
                    duplicateTCmd = true;
                    txPayload(OK);
                } else {
                    m600RunoutChanging = false;
                    mmuFSensorLoading = true;
                    duplicateTCmd = false;
                    toolChange(tData2);
                    if (load_filament_at_toolChange) {
                        load_filament_withSensor();
                        load_filament_at_toolChange = false;
                        txPayload(OK);
                    }
                }
            }
        } else if (tData1 == 'L') {
            // Lx Load Filament CMD Received
            if ((tData2 >= 0) && (tData2 < EXTRUDERS) && !isFilamentLoaded) {
                set_positions(active_extruder, tData2, true);
                delay(10);
                feed_filament();
                delay(100);
                txPayload(OK);
            }
        } else if ((tData1 == 'U') && (tData2 == '0')) {
            // Ux Unload filament CMD Received
            unload_filament_withSensor(active_extruder);
            txPayload(OK);
            isPrinting = false;
            trackToolChanges = 0;
            //disableAllSteppers();
        } else if (tData1 == 'S') {
            // Sx Starting CMD Received
            if (tData2 == '0') {
                txPayload(OK);
            } else if (tData2 == '1') {
                unsigned char tempS1[3] = {(FW_VERSION >> 8), (0xFF & FW_VERSION), BLK};
                txPayload(tempS1);
            } else if (tData2 == '2') {
                unsigned char tempS2[3] = {(FW_BUILDNR >> 8), (0xFF & FW_BUILDNR), BLK};
                txPayload(tempS2);
            }
        } else if (tData1 == 'M') {
            // Mx Modes CMD Received
            // M0: set to normal mode; M1: set to stealth mode
            if (tData2 == '0') tmc2130_mode =  NORMAL_MODE;
            if (tData2 == '1') tmc2130_mode = STEALTH_MODE;
            //init all axes
            tmc2130_init(tmc2130_mode);
            txPayload(OK);
        } else if ((tData1 == 'F') && (tData2 == 'S')) {
            // FS Filament Seen by MK3-FSensor CMD Received
            fsensor_triggered = true;
            // OK is sent once filament @ Bondtech Gears
        } else if (tData1 == 'F') {
            // Fxy Filament Type Set CMD Received
            if (((tData2 >= 0) && (tData2 < EXTRUDERS)) && ((tData3 >= 0) && (tData3 <= 2))) {
                filament_type[(int)tData2] = (int)tData3;
                txPayload(OK);
            }
        } else if ((tData1 == 'X') && (tData2 == '0')) {
            // Xx RESET CMD Received
            wdt_enable(WDTO_15MS);
        } else if ((tData1 == 'P') && (tData2 == '0')) {
            // P0 Read FINDA CMD Received
            byte txTemp[3] = {'O', 'K', digitalRead(A1)};
            txPayload(txTemp);
        } else if ((tData1 == 'C') && (tData2 == '0')) {
            // Cx Continue Load onto Bondtech Gears CMD Received
            if (!duplicateTCmd) {
                load_filament_into_extruder();
                txPayload(OK);
            } else txPayload(OK);
        } else if (tData1 == 'E') {
            // Ex Eject Filament X CMD Received
            if ((tData2 >= 0) && (tData2 < EXTRUDERS)) { // Ex: eject filament
                m600RunoutChanging = true;
                eject_filament(tData2);
                txPayload(OK);
            }
        } else if ((tData1 == 'R') && (tData2 == '0')) {
            // Rx Recover Post-Eject Filament X CMD Received
            recover_after_eject();
            txPayload(OK);
        } else if (!mmuFSensorLoading && fsensor_triggered) {
            fsensor_triggered = false;
            txPayload(OK);
        } // End of Processing Commands
    }     // End of Confirmed with Valid CSUM
}


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
void fixTheProblem(bool showPrevious) {

    engage_filament_pulley(false);                    // park the idler stepper motor
    shr16_clr_ena(AX_SEL);                            // turn OFF the selector stepper motor
    shr16_clr_ena(AX_IDL);                            // turn OFF the idler stepper motor
    //fixingTheProblems = true;

    while ((Btn::middle != buttonClicked()) || digitalRead(A1)) {
        //process_commands();
        //  wait until key is entered to proceed  (this is to allow for operator intervention)
        if (!showPrevious) {
            delay(100);
            shr16_clr_led();
            delay(100);
            if (digitalRead(A1)) {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            } else shr16_set_led(1 << 2 * (4 - active_extruder));
        } else {
            delay(100);
            shr16_clr_led();
            shr16_set_led(1 << 2 * (4 - active_extruder));
            delay(100);
            if (digitalRead(A1)) {
                shr16_set_led(2 << 2 * (4 - previous_extruder));
            } else shr16_set_led(1 << 2 * (4 - previous_extruder));
        }
    }
    //fixingTheProblems = false;
    tmc2130_init_axis(AX_SEL, tmc2130_mode);           // turn ON the selector stepper motor
    tmc2130_init_axis(AX_IDL, tmc2130_mode);           // turn ON the idler stepper motor
    home(true); // Home and return to previous active extruder
}
