// shr16.c - 16bit shift register (2x74595)

#include "shr16.h"
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
    PORTB &= ~0x40;
    asm("nop");
    uint16_t m;
    for (m = 0x8000; m; m >>= 1) {
        if (m & v) {
            PORTB |= 0x20;
        } else {
            PORTB &= ~0x20;
        }
        PORTC |= 0x80;
        asm("nop");
        PORTC &= ~0x80;
        asm("nop");
    }
    PORTB |= 0x40;
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
    shr16_write((shr16_v & ~SHR16_LED_MSK) | led);
}

/**
 * @brief shr16_set_ena
 * Enable stepper driver of each channel
 * @param ena: bit mask with bit0 = axis0, ..., bit2 = axis2
 * 1 = enable, 0 = disable
 */
void shr16_set_ena(uint8_t ena)
{
    ena ^= 7;
    ena = ((ena & 1) << 1) | ((ena & 2) << 2) | ((ena & 4) << 3);
    shr16_write((shr16_v & ~SHR16_ENA_MSK) | ena);
}

/**
 * @brief shr16_set_dir
 * Set direction signals of stepper drivers
 * @param dir: bit mask with bit0 = axis0, ..., bit2 = axis2
 */
void shr16_set_dir(uint8_t dir)
{
    dir = (dir & 1) | ((dir & 2) << 1) | ((dir & 4) << 2);
    shr16_write((shr16_v & ~SHR16_DIR_MSK) | dir);
}

uint8_t shr16_get_ena(void)
{
    return ((shr16_v & 2) >> 1) | ((shr16_v & 8) >> 2) | ((shr16_v & 0x20) >> 3);
}

uint8_t shr16_get_dir(void)
{
    return (shr16_v & 1) | ((shr16_v & 4) >> 1) | ((shr16_v & 0x10) >> 2);
}
