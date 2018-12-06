// abtn3.c - 3 buttons on analog channel

#include "abtn3.h"
#include "adc.h"

// public variables:
uint8_t abtn_state = 0;
uint8_t abtn_click = 0;

inline uint8_t abtn3_sample(void)
{
    int raw = adc_val[0];
    // Button 1 - 0
    // Button 2 - 344
    // Button 3 - 516
    if (raw < 10) {
        return 1;
    } else if (raw > 320 && raw < 360) {
        return 2;
    } else if (raw > 500 && raw < 530) {
        return 4;
    }
    return (0);
}

uint8_t abtn3_update(void)
{
    uint8_t state = abtn3_sample();
    abtn_click |= ~state & abtn_state;
    abtn_state = state;
    return abtn_click;
}

uint8_t abtn3_clicked(uint8_t btn)
{
    uint8_t clicked = abtn_click & (1 << btn);
    abtn_click &= ~clicked;
    return clicked ? 1 : 0;
}
