//! @file
//! @author Marek Bel

#ifndef PERMANENT_STORAGE_H_
#define PERMANENT_STORAGE_H_

#include <stdint.h>

//! @brief Read manipulate and store bowden length
//!
//! Value is stored independently for each filament.
//! Active filament is deduced from active_extruder global variable.
class BowdenLength
{
public:
    static uint16_t get();
    static void eraseAll();
    static const uint8_t stepSize = 10u; //!< increase()/decrease() bowden length step size
    BowdenLength();
    bool increase();
    bool decrease();
    ~BowdenLength();
private:
    uint8_t m_filament; //!< Selected filament
    uint16_t m_length;  //!< Selected filament bowden length
};

#endif /* PERMANENT_STORAGE_H_ */
