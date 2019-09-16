//! @file
//! @author Marek Bel

#ifndef PERMANENT_STORAGE_H_
#define PERMANENT_STORAGE_H_

#include <stdint.h>

void permanentStorageInit();

void eepromEraseAll();

//! @brief Read manipulate and store bowden length
//!
//! Value is stored independently for each filament.
//! Active filament is deduced from active_extruder global variable.
class BowdenLength
{
private:
    uint8_t m_filament; //!< Selected filament
public:
    static uint16_t get();
    static uint16_t getFSensorSteps();
    static const uint8_t stepSize = 38u; //!< increase()/decrease() bowden length step size
    static const uint8_t stepSizeFSensor = 19u; //!< increase()/decrease() bowden length step size
    bool increase();
    bool decrease();
    ~BowdenLength();
    uint16_t m_length;  //!< Selected filament bowden length
    BowdenLength();
};

//! @brief Read and store last filament loaded to nozzle
//!
//! 800(data) + 3(status) EEPROM cells are used to store 4 bit value frequently
//! to spread wear between more cells to increase durability.
//!
//! Expected worst case durability scenario:
//! @n Print has 240mm height, layer height is 0.1mm, print takes 10 hours,
//!    filament is changed 5 times each layer, EEPROM endures 100 000 cycles
//! @n Cell written per print: 240/0.1*5/800 = 15
//! @n Cell written per hour : 15/10 = 1.5
//! @n First cell failure expected: 100 000 / 1.5 = 66 666 hours = 7.6 years
//!
//! Algorithm can handle one cell failure in status and one cell in data.
//! Status use 2 of 3 majority vote.
//! If bad data cell is detected, status is switched to next key.
//! Key alternates between begin to end and end to begin write order.
//! Two keys are needed for each start point and direction.
//! If two data cells fails, area between them is unavailable to write.
//! If this is first and last cell, whole storage is disabled.
//! This vulnerability can be avoided by adding additional keys
//! and start point in the middle of the EEPROM.
//!
//! It would be possible to implement twice as efficient algorithm, if
//! separate EEPROM erase and EEPROM write commands would be available and
//! if write command would allow to be invoked twice between erases to update
//! just one nibble. Such commands are not available in AVR Libc, and possibility
//! to use write command twice is not documented in atmega32U4 datasheet.
//!
class FilamentLoaded
{
public:
    static bool get(uint8_t &filament);
    static bool set(uint8_t filament);
private:
    enum Key
    {
        KeyFront1,
        KeyReverse1,
        KeyFront2,
        KeyReverse2,
        BehindLastKey,
    };
    static_assert (BehindLastKey - 1 <= 0xf, "Key does't fit into nibble.");
    static uint8_t getStatus();
    static bool setStatus(uint8_t status);
    static int16_t getIndex();
    static void getNext(uint8_t &status, int16_t &index);
    static void getNext(uint8_t &status);
};


#endif /* PERMANENT_STORAGE_H_ */
