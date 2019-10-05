// Detect12V24V.cpp

#include "Detect12V24V.h"

const uint8_t VoltagePIN = A0; // A2D is a divide by 10 setup on PIN A0

/** This is used to determine the supply voltage to the MMU2S
 *  When system is on 12v only stealth mode is allowed to be active
 *  Even though the printer only allows it to be set this way I 
 *  think it's a good idea.
 * 
 *   Output is 10 x actual so 120 is 12v and 240 is 24v
 * 
 */
uint8_t getMMU2S_System_Voltage(void)
{
    uint32_t reading = 0;
    for (int i=0; i < 4; i++) reading += analogRead(VoltagePIN);
    return (reading/8.5);
}