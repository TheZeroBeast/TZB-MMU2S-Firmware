// shr16.c - 16bit shift register (2x74595)

#include "shr16.h"
#include "config.h"
#include <avr/io.h>

// public variables:
uint16_t shr16_v;

void shr16_init(void)
{
    DDRC |= 0x80;
    DDRB |= 0x40;
    DDRB |= 0x20;
    PORTC &= ~0x80;
    PORTB &= ~0x40;
    PORTB &= ~0x20;
    shr16_v = 0;
    shr16_write(shr16_v);
    shr16_write(shr16_v);
}

void shr16_write(uint16_t v)
{
    PORTB &= ~0x40;                 // Pull low PORTB6 to D10 on Shift Register CS
    asm("nop");
    uint16_t m;
    for (m = 0x8000; m; m >>= 1) {  // Step through register updating
        if (m & v) {
            PORTB |= 0x20;          // Pull high data line for one clock pulse D9
        } else {
            PORTB &= ~0x20;         // Pull low data line for one clock pulse D9
        }
        PORTC |= 0x80;              // C7 clock is synced
        asm("nop");
        PORTC &= ~0x80;
        asm("nop");
    }
    PORTB |= 0x40;                  // Replace high PORTB6 to D10 Shift Register CS
    asm("nop");
    shr16_v = v;
}

/**
 * @brief shr16_set_led
 * Enable LEDs, active high
 *
 * @param led: bit mask with
 * // TODO 2: double check documentation
 *   bit0 = green led of last extruder (0)
 *   bit0 = red led of last extruder (0)
 *   alternating green-red until
 *   bit9 = red LED of first extruder (4)
 */
void shr16_set_led(uint16_t led) // TODO 2: provide macros with easily readable names
{
    led = ((led & 0x00ff) << 8) | ((led & 0x0300) >> 2);
    shr16_write(shr16_v | led);
}

void shr16_clr_led(void)
{
    shr16_write(shr16_v & ~SHR16_LED_MSK);
}

void shr16_set_ena(int axis)
{
    switch (axis) {
    case AX_PUL:
        shr16_write(shr16_v & ~SHR16_ENA_PUL);
        break;
    case AX_SEL:
        shr16_write(shr16_v & ~SHR16_ENA_SEL);
        break;
    case AX_IDL:
        shr16_write(shr16_v & ~SHR16_ENA_IDL);
    }
}

void shr16_set_ena_all(void)
{
    // Clear enable pins then set them on
    shr16_write(shr16_v & ~SHR16_ENA_MSK); // | SHR16_DIR_MSK));
}

void shr16_clr_ena(int axis)
{
    switch (axis) {
    case AX_PUL:
        shr16_write(shr16_v | SHR16_ENA_PUL);
        break;
    case AX_SEL:
        shr16_write(shr16_v | SHR16_ENA_SEL);
        break;
    case AX_IDL:
        shr16_write(shr16_v | SHR16_ENA_IDL);
    }
}

void shr16_clr_ena_all(void)
{
    shr16_write(shr16_v | SHR16_ENA_MSK);
}

/**
 * @brief shr16_set_dir
 * Set direction signals of stepper drivers
 * @param dir: bit mask with bit0 = axis0, ..., bit2 = axis2
 */
void shr16_set_dir(uint8_t dir)
{
    dir = (dir & 1) | ((dir & 2) << 1) | ((dir & 4) << 2);  // 0., 1. << 1 == 2., 2. << 2 == 4.
    shr16_write((shr16_v & ~SHR16_DIR_MSK) | dir);
}

uint8_t shr16_get_ena(void)
{
    return ((shr16_v & SHR16_ENA_PUL) >> 1) | ((shr16_v & SHR16_ENA_SEL) >> 2) | ((shr16_v & SHR16_ENA_IDL) >> 3);
}

uint8_t shr16_get_dir(void)
{
    return (shr16_v & 1) | ((shr16_v & 4) >> 1) | ((shr16_v & 0x10) >> 2);
}
