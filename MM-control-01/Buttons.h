// Buttons.h

#ifndef _BUTTONS_h
#define _BUTTONS_h

#include <stdint.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

enum class Btn : uint8_t {
    none = 0,
    right = 1,
    middle = 2,
    left = 4,
};

inline Btn operator|(Btn a, Btn b)
{
    return static_cast<Btn>( static_cast<uint8_t>(a) | static_cast<uint8_t>(b) );
}

inline bool operator&(Btn a, Btn b)
{
    return static_cast<uint8_t>(a) & static_cast<uint8_t>(b);
}

void setupMenu();
Btn buttonClicked();

#endif //_BUTTONS_h
