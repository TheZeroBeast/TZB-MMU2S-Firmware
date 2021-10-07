//! @file

#include "main.h"

// public variables:
bool MMU2SLoading = false;
bool m600RunoutChanging = false;
//bool duplicateTCmd = false;
bool inErrorState = false;
long startWakeTime;
void process_commands(void);

uint8_t tmc2130_mode = STEALTH_MODE;

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
    startWakeTime = millis();
    
    led_blink(1);
    
    Serial.begin(UART0_BDR, SERIAL_8N2); //serial0 - USB
    
    //Setup USART1 Interrupt Registers
    UCSR1A = (0 << U2X1); // baudrate multiplier
    UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (0 << UCSZ12);   // Turn on the transmission and reception circuitry
    UCSR1C = (0 << UMSEL11) | (0 << UMSEL10)| (0 << UPM11)
             | (0 << UPM10) | (1 << USBS1) | (1 << UCSZ11) | (1 << UCSZ10); // Use 8-bit character sizes
    UCSR1D = (0 << CTSEN) | (0 << RTSEN); // Disable flow control
    UBRR1H = (BAUD_PRESCALE >> 8); // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
    UBRR1L = BAUD_PRESCALE; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register
    UCSR1B |= (1 << RXCIE1);

    PORTF |= 0x20; // Set Button ADC Pin High
    
    sei();

    led_blink(2);
    spi_init();
    led_blink(3);
    led_blink(4);

    shr16_clr_led();
    homeIdlerSmooth(true);
    tmc2130_init(tmc2130_mode);
    if (active_extruder != EXTRUDERS) txPayload((unsigned char*)"STR--");
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

    if (!isFilamentLoaded()) {
        switch (buttonClicked()) {
        case ADC_Btn_Right:
            if (active_extruder < EXTRUDERS) set_positions(active_extruder + 1, true);
            if (active_extruder == EXTRUDERS) txPayload((unsigned char*)"X1---");
            break;
        case ADC_Btn_Left:
            if (active_extruder == EXTRUDERS) txPayload((unsigned char*)"ZZZ--");
            if (active_extruder > 0) set_positions(active_extruder - 1, true);
            break;
        default:
            break;
        }
    } else {
        switch (buttonClicked()) {
          case ADC_Btn_Right:
          case ADC_Btn_Left:
            txPayload((unsigned char*)"Z1---");
            delay(1000);
            txPayload((unsigned char*)"ZZZ--");
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
    if (!isPrinting && !isEjected) {
        manual_extruder_selector();
        if (ADC_Btn_Middle == buttonClicked()) {
            if (active_extruder < EXTRUDERS) feed_filament();
            else if (active_extruder == EXTRUDERS) 
            {
                txPayload((unsigned char*)"SETUP");
                setupMenu();
            }
        }
    } else if (isEjected) {
        switch (buttonClicked()) {
        case ADC_Btn_Right:
            engage_filament_pulley(true);
            moveSmooth(AX_PUL, (EJECT_PULLEY_STEPS * -1),
        filament_lookup_table[5][filament_type[previous_extruder]], false, false, GLOBAL_ACC);
            engage_filament_pulley(false);
            break;
        default:
            break;
        }
    }

    if (((millis() - startWakeTime) > WAKE_TIMER) && !isFilamentLoaded() &&
         !isPrinting && (shr16_get_ena() != 111)) disableAllSteppers();
}

void process_commands(void)
{
    cli();
    // Copy volitale vars as local
    unsigned char tData1 = rxData1;
    unsigned char tData2 = rxData2;
    unsigned char tData3 = rxData3;
    // Currently unused. unsigned char tData4 = rxData4;
    // Currently unused. unsigned char tData5 = rxData5;
    bool confPayload = confirmedPayload;
    if (confPayload) confirmedPayload = false;
    else { tData1 = ' '; tData2 = ' '; tData3 = ' '; // Currently unused. tData4 = ' '; tData5 = ' ';
    } sei();
    if (inErrorState) return;

    if (tData1 == 'T') {
        //Tx Tool Change CMD Received
        if (tData2 < EXTRUDERS) {
            m600RunoutChanging = false;
            MMU2SLoading = true;
            toolChange(tData2);
            txPayload(OK);
        }
    } else if (tData1 == 'L') {
        // Lx Load Filament CMD Received
        if (tData2 < EXTRUDERS) {
            if (isFilamentLoaded()) {
                txPayload((unsigned char*)"Z1---");
                delay(1500);
                txPayload((unsigned char*)"ZZZ--");
            } else {
                set_positions(tData2, true);
                feed_filament(); // returns OK and active_extruder to update MK3
            }
            txPayload(OK);
        }
    } else if ((tData1 == 'U') && (tData2 == '0')) {
        // Ux Unload filament CMD Received
        unload_filament_withSensor();
        homedOnUnload = false; // Clear this flag as unload_filament_withSensor() method uses it within the 'T' cmds
        txPayload(OK);
        isPrinting = false;
        toolChanges = 0;
        trackToolChanges = 0;
    } else if (tData1 == 'S') {
        // Sx Starting CMD Received
        if (tData2 == '0') {
            txPayload(OK);
        } else if (tData2 == '1') {
            unsigned char tempS1[5] = {'O','K',(FW_VERSION >> 8), (0xFF & FW_VERSION), BLK};
            txPayload(tempS1);
        } else if (tData2 == '2') {
            unsigned char tempS2[5] = {'O','K',(FW_BUILDNR >> 8), (0xFF & FW_BUILDNR), BLK};
            txPayload(tempS2);
        } else if (tData2 == '3') {
            unsigned char tempS3[5] = {'O','K',(uint8_t)active_extruder, BLK, BLK};
            txPayload(tempS3);
        }
    } else if (tData1 == 'M') {
            // Mx Modes CMD Received
            // M0: set to normal mode; M1: set to stealth mode
            uint8_t sysV = getMMU2S_System_Voltage();
            // Only allow NORMAL mode if voltage is between 18v and 26v
            if (tData2 == 0 && (sysV < 254 && sysV > 180)) {
                filament_lookup_table[0][0] = TYPE_0_MAX_SPPED_PUL;
                filament_lookup_table[0][1] = TYPE_1_MAX_SPPED_PUL;
                filament_lookup_table[0][2] = TYPE_2_MAX_SPPED_PUL;
                MAX_SPEED_IDLER = MAX_SPEED_IDL_DEF_NORMAL;
                MAX_SPEED_SELECTOR = MAX_SPEED_SEL_DEF_NORMAL;
                GLOBAL_ACC = GLOBAL_ACC_DEF_NORMAL;
                tmc2130_mode =  NORMAL_MODE;
            }
            if (tData2 == 1) {
                for (uint8_t i = 0; i < 3; i++)
                    if (filament_lookup_table[0][i] > 1500) filament_lookup_table[0][i] = 1400;
                MAX_SPEED_IDLER = MAX_SPEED_IDL_DEF_STEALTH;
                MAX_SPEED_SELECTOR = MAX_SPEED_SEL_DEF_STEALTH;
                GLOBAL_ACC = GLOBAL_ACC_DEF_STEALTH;
                tmc2130_mode = STEALTH_MODE;
            }
            tmc2130_init(tmc2130_mode);
            unsigned char tempOKM[5] = {'O','K','M', tData2, BLK};
            txPayload(tempOKM);
            return;
    } else if (tData1 == 'F') {
        // Fxy Filament Type Set CMD Received
        if ((tData2 < EXTRUDERS) && (tData3 < 3)) {
            filament_type[tData2] = tData3;
            txPayload(OK);
        }
    } else if ((tData1 == 'X') && (tData2 == '0')) {
        // Xx RESET CMD Received (Done by starting program again)
        asm("jmp 0");
    } else if ((tData1 == 'P') && (tData2 == '0')) {
        txFINDAStatus();
    } else if ((tData1 == 'C') && (tData2 == '0')) {
        txPayload(OK);
        load_filament_into_extruder();
    } else if  (tData1 == 'E') {
        // Ex Eject Filament X CMD Received
        if (tData2 < EXTRUDERS) { // Ex: eject filament
            m600RunoutChanging = true;
            eject_filament(tData2);
            txPayload(OK);
        }
    } else if ((tData1 == 'R') && (tData2 == '0')) {
        // Rx Recover Post-Eject Filament X CMD Received
        recover_after_eject();
        txPayload(OK);
    } else if (tData1 == 'K') {
        // Kx Cut filament
        if (tData2 < EXTRUDERS) {
            mmctl_cut_filament(tData2);
            txPayload(OK);
        }
    }
}

//****************************************************************************************************
//* this routine is the common routine called for fixing the filament issues (loading or unloading)
//****************************************************************************************************
void fixTheProblem(bool showPrevious) {
    engage_filament_pulley(false);                    // park the idler stepper motor
    shr16_clr_ena(AX_SEL);                            // turn OFF the selector stepper motor
    shr16_clr_ena(AX_IDL);                            // turn OFF the idler stepper motor

    inErrorState = true;

    while ((ADC_Btn_Middle != buttonClicked()) || isFilamentLoaded()) {
        //  wait until key is entered to proceed  (this is to allow for operator intervention)
        if (!showPrevious) {
            switch (buttonClicked()) {
                case ADC_Btn_Right:
                    engage_filament_pulley(true);
                    if (isFilamentLoaded()) {
                        if (moveSmooth(AX_PUL, ((BOWDEN_LENGTH * 1.5) * -1),
                                       filament_lookup_table[5][filament_type[active_extruder]],
                                       false, false, GLOBAL_ACC, true) == MR_Success) { // move to trigger FINDA
                            moveSmooth(AX_PUL, filament_lookup_table[3][filament_type[active_extruder]],
                                       filament_lookup_table[5][filament_type[active_extruder]], false, false, GLOBAL_ACC); // move to filament parking position
                        }
                    } else moveSmooth(AX_PUL, -300, filament_lookup_table[5][filament_type[active_extruder]], false);
                    engage_filament_pulley(false);
                    shr16_clr_ena(AX_SEL);                    
                    shr16_clr_ena(AX_IDL);
                    break;
                case ADC_Btn_Left:
                    engage_filament_pulley(true);
                    moveSmooth(AX_PUL, 300, filament_lookup_table[5][filament_type[previous_extruder]]*1.8, false);
                    engage_filament_pulley(false);
                    shr16_clr_ena(AX_SEL);                    
                    shr16_clr_ena(AX_IDL);
                    break;
                default:
                    break;
            }
            delay(100);
            shr16_clr_led();
            delay(100);
            if (isFilamentLoaded()) {
                shr16_set_led(2 << 2 * (4 - active_extruder));
            } else shr16_set_led(1 << 2 * (4 - active_extruder));
        } else {
            switch (buttonClicked()) {
                case ADC_Btn_Right:
                    engage_filament_pulley(true);
                    if (isFilamentLoaded()) {
                        if (moveSmooth(AX_PUL, ((BOWDEN_LENGTH * 1.5) * -1),
                                       filament_lookup_table[5][filament_type[previous_extruder]]*1.8,
                                       false, false, GLOBAL_ACC, true) == MR_Success) { // move to trigger FINDA
                            moveSmooth(AX_PUL, filament_lookup_table[3][filament_type[previous_extruder]],
                                       filament_lookup_table[5][filament_type[previous_extruder]]*1.8, false, false, GLOBAL_ACC); // move to filament parking position
                        }
                    } else moveSmooth(AX_PUL, -300, filament_lookup_table[5][filament_type[previous_extruder]]*1.8, false);
                    engage_filament_pulley(false);
                    shr16_clr_ena(AX_IDL);
                    break;
                case ADC_Btn_Left:
                    engage_filament_pulley(true);
                    moveSmooth(AX_PUL, 300, filament_lookup_table[5][filament_type[previous_extruder]]*1.8, false);
                    engage_filament_pulley(false);
                    shr16_clr_ena(AX_IDL);
                    break;
                default:
                    break;
            }
            delay(100);
            shr16_clr_led();
            if (active_extruder != previous_extruder) shr16_set_led(1 << 2 * (4 - active_extruder));
            delay(100);
            if (isFilamentLoaded()) {
                shr16_set_led(2 << 2 * (4 - previous_extruder));
            } else shr16_set_led(1 << 2 * (4 - previous_extruder));
        }
    }
    delay(100);
    tmc2130_init_axis(AX_SEL, tmc2130_mode);           // turn ON the selector stepper motor
    tmc2130_init_axis(AX_IDL, tmc2130_mode);           // turn ON the idler stepper motor
    inErrorState = false;
    txPayload((unsigned char*)"ZZZ--"); // Clear MK3 Message
    home(true); // Home and return to previous active extruder
    trackToolChanges = 0;
}

void fixSelCrash(void) {

    engage_filament_pulley(false);                    // park the idler stepper motor
    shr16_clr_ena(AX_SEL);                            // turn OFF the selector stepper motor
    inErrorState = true;

    while (ADC_Btn_Middle != buttonClicked()) {
        //  wait until key is entered to proceed  (this is to allow for operator intervention)
        delay(100);
        shr16_clr_led();
        delay(100);
        if (isFilamentLoaded()) {
            shr16_set_led(2 << 2 * (4 - active_extruder));
        } else shr16_set_led(1 << 2 * (4 - active_extruder));
    }
    selSGFailCount = 0;
    inErrorState = false;    
    set_sel_toLast_positions(active_extruder);
}

void fixIdlCrash(void) {

    shr16_clr_ena(AX_IDL);                            // turn OFF the idler stepper motor
    inErrorState = true;

    while (ADC_Btn_Middle != buttonClicked()) {
        //  wait until key is entered to proceed  (this is to allow for operator intervention)
        delay(100);
        shr16_clr_led();
        delay(100);
        if (isFilamentLoaded()) {
            shr16_set_led(2 << 2 * (4 - active_extruder));
        } else shr16_set_led(1 << 2 * (4 - active_extruder));
    }
    idlSGFailCount = 0;
    inErrorState = false;
    set_idler_toLast_positions(active_extruder);
}
