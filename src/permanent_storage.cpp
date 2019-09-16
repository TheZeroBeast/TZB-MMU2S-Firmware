//! @file
//! @author Marek Bel

#include "permanent_storage.h"
#include "mmctl.h"
#include "config.h"
#include <avr/eeprom.h>

#define ARR_SIZE(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

//! @brief EEPROM data layout
//!
//! Do not remove, reorder or change size of existing fields.
//! Otherwise values stored with previous version of firmware would be broken.
//! It is possible to add fields in the end of this struct, ensure that erased EEPROM is handled well.
//! Last byte in EEPROM is reserved for layoutVersion. If some field is repurposed, layoutVersion
//! needs to be changed to force EEPROM erase.
typedef struct __attribute__ ((packed))
{
    uint8_t eepromLengthCorrection; //!< legacy bowden length correction
    uint16_t eepromBowdenLen[5];    //!< Bowden length for each filament
    uint8_t eepromFilamentStatus[3];//!< Majority vote status of eepromFilament wear leveling
    uint8_t eepromFilament[800];    //!< Top nibble status, bottom nibble last filament loaded
}
eeprom_t;
static_assert(sizeof(eeprom_t) - 2 <= E2END, "eeprom_t doesn't fit into EEPROM available.");
//! @brief EEPROM layout version
static const uint8_t layoutVersion = 0xff;

static eeprom_t * const eepromBase = reinterpret_cast<eeprom_t*>(0); //!< First EEPROM address
static const uint16_t eepromEmpty = 0xffff; //!< EEPROM content when erased
static const uint16_t eepromLengthCorrectionBase = 7900u; //!< legacy bowden length correction base
static const uint16_t eepromBowdenLenDefault = 8000u; //!< Default bowden length
static const uint16_t eepromBowdenLenMinimum = 6900u; //!< Minimum bowden length
static const uint16_t eepromBowdenLenMaximum = 65500u; //!< Maximum bowden length
static const uint16_t eepromFSensorStepsDefault = 290u; //!< Default bowden length

void permanentStorageInit()
{
    if (eeprom_read_byte((uint8_t*)E2END) != layoutVersion) eepromEraseAll();
}

//! @brief Erase whole EEPROM
void eepromEraseAll()
{
    for (uint16_t i = 0; i < E2END; i++)
    {
        eeprom_update_byte((uint8_t*)i, static_cast<uint8_t>(eepromEmpty));
    }
    eeprom_update_byte((uint8_t*)E2END, layoutVersion);
}

//! @brief Is filament number valid?
//! @retval true valid
//! @retval false invalid
static bool validFilament(uint8_t filament)
{
    if (filament < (EXTRUDERS-1)) return true;
    else return false;
}

//! @brief Is bowden length in valid range?
//! @param BowdenLength bowden length
//! @retval true valid
//! @retval false invalid
static bool validBowdenLen (const uint16_t BowdenLength)
{
    if ((BowdenLength >= eepromBowdenLenMinimum)
            && BowdenLength <= eepromBowdenLenMaximum) return true;
    return false;
}

//! @brief Get bowden length for active filament
//!
//! Returns stored value, doesn't return actual value when it is edited by increase() / decrease() unless it is stored.
//! @return stored bowden length
uint16_t BowdenLength::get()
{
    uint8_t filament = 0;
    if (validFilament(filament))
    {
        uint16_t bowdenLength = eeprom_read_word(&(eepromBase->eepromBowdenLen[filament]));

        if (eepromEmpty == bowdenLength)
        {
            const uint8_t LengthCorrectionLegacy = eeprom_read_byte(&(eepromBase->eepromLengthCorrection));
            if (LengthCorrectionLegacy <= 200)
            {
                bowdenLength = eepromLengthCorrectionBase + LengthCorrectionLegacy * 10;
            }
        }
        if (validBowdenLen(bowdenLength)) return bowdenLength;
    }

    return eepromBowdenLenDefault;
}

//! @brief Construct BowdenLength object which allows bowden length manipulation
//!
//! To be created on stack, new value is permanently stored when object goes out of scope.
//! Active filament and associated bowden length is stored in member variables.
BowdenLength::BowdenLength() : m_filament(0), m_length(BowdenLength::get())
{
}

//! @brief Increase bowden length
//!
//! New value is not stored immediately. See ~BowdenLength() for storing permanently.
//! @retval true passed
//! @retval false failed, it is not possible to increase, new bowden length would be out of range
bool BowdenLength::increase()
{
    if ( validBowdenLen(m_length + stepSize))
    {
        m_length += stepSize;
        return true;
    }
    return false;
}

//! @brief Decrease bowden length
//!
//! New value is not stored immediately. See ~BowdenLength() for storing permanently.
//! @retval true passed
//! @retval false failed, it is not possible to decrease, new bowden length would be out of range
bool BowdenLength::decrease()
{
    if ( validBowdenLen(m_length - stepSize))
    {
        m_length -= stepSize;
        return true;
    }
    return false;
}

//! @brief Store bowden length permanently.
BowdenLength::~BowdenLength()
{
    if (validFilament(m_filament))eeprom_update_word(&(eepromBase->eepromBowdenLen[m_filament]), m_length);
}

//! @brief Get filament storage status
//!
//! Uses 2 out of 3 majority vote.
//!
//! @return status
//! @retval 0xff Uninitialized EEPROM or no 2 values agrees
uint8_t FilamentLoaded::getStatus()
{
    if (eeprom_read_byte(&(eepromBase->eepromFilamentStatus[0])) == eeprom_read_byte(&(eepromBase->eepromFilamentStatus[1])))
        return eeprom_read_byte(&(eepromBase->eepromFilamentStatus[0]));
    if (eeprom_read_byte(&(eepromBase->eepromFilamentStatus[0])) == eeprom_read_byte(&(eepromBase->eepromFilamentStatus[2])))
        return eeprom_read_byte(&(eepromBase->eepromFilamentStatus[0]));
    if (eeprom_read_byte(&(eepromBase->eepromFilamentStatus[1])) == eeprom_read_byte(&(eepromBase->eepromFilamentStatus[2])))
        return eeprom_read_byte(&(eepromBase->eepromFilamentStatus[1]));
    return 0xff;
}

//! @brief Set filament storage status
//!
//! @retval true Succeed
//! @retval false Failed
bool FilamentLoaded::setStatus(uint8_t status)
{
    for (uint8_t i = 0; i < ARR_SIZE(eeprom_t::eepromFilamentStatus); ++i)
    {
        eeprom_update_byte(&(eepromBase->eepromFilamentStatus[i]), status);
    }
    if (getStatus() == status) return true;
    return false;
}


//! @brief Get index of last valid filament
//!
//! Depending on status, it searches from the beginning or from the end of eepromFilament[]
//! for the first non-matching status. Previous index (of matching status, or out of array bounds)
//! is returned.
//!
//! @return index to eepromFilament[] of last valid value
//! it can be out of array range, if first item status doesn't match expected status
//! getNext(index, status) turns it to first valid index.
int16_t FilamentLoaded::getIndex()
{
    const uint8_t status = getStatus();
    int16_t index = -1;
    switch (status)
    {
    case KeyFront1:
    case KeyFront2:
        index = ARR_SIZE(eeprom_t::eepromFilament) - 1; // It is the last one, if no dirty index found
        for(uint16_t i = 0; i < ARR_SIZE(eeprom_t::eepromFilament); ++i)
        {
            if (status != (eeprom_read_byte(&(eepromBase->eepromFilament[i])) >> 4 ))
            {
                index = i - 1;
                break;
            }
        }
        break;
    case KeyReverse1:
    case KeyReverse2:
        index = 0; // It is the last one, if no dirty index found
        for(int16_t i = (ARR_SIZE(eeprom_t::eepromFilament) - 1); i >= 0; --i)
        {
            if (status != (eeprom_read_byte(&(eepromBase->eepromFilament[i])) >> 4 ))
            {
                index = i + 1;
                break;
            }
        }
        break;
    default:
        break;
    }
    return index;
}

//! @brief Get last filament loaded
//! @par [in,out] filament filament number 0 to 4
//! @retval true success
//! @retval false failed
bool FilamentLoaded::get(uint8_t& filament)
{
    int16_t index = getIndex();
    if ((index < 0) || (static_cast<uint16_t>(index) >= ARR_SIZE(eeprom_t::eepromFilament))) return false;
    const uint8_t rawFilament = eeprom_read_byte(&(eepromBase->eepromFilament[index]));
    filament = 0x0f & rawFilament;
    if (filament > 4) return false;
    const uint8_t status = getStatus();
    if (!(status == KeyFront1
            || status == KeyReverse1
            || status == KeyFront2
            || status == KeyReverse2)) return false;
    if ((rawFilament >> 4) != status) return false;
    return true;
}

//! @brief Set filament being loaded
//!
//! Always fails, if it is not possible to store status.
//! If it is not possible store filament, it tries all other
//! keys. Fails if storing with all other keys failed.
//!
//! @par filament bottom 4 bits are stored
//! but only value 0 to 4 passes validation in FilamentLoaded::get()
//! @retval true success
//! @retval false failed
bool FilamentLoaded::set(uint8_t filament)
{
    for (uint8_t i = 0; i < BehindLastKey - 1 ; ++i)
    {
        uint8_t status = getStatus();
        int16_t index = getIndex();
        getNext(status, index);
        if(!setStatus(status)) return false;
        uint8_t filamentRaw = ((status << 4) & 0xf0) + (filament & 0x0f);
        eeprom_update_byte(&(eepromBase->eepromFilament[index]), filamentRaw);
        if (filamentRaw == eeprom_read_byte(&(eepromBase->eepromFilament[index]))) return true;
        getNext(status);
        if(!setStatus(status)) return false;
    }
    return false;
}

//! @brief Get next status and index
//!
//! Get next available index following index input parameter to store filament in eepromFilament[].
//! If index would reach behind indexable space, status is updated to next and first index matching status indexing mode is returned.
//! @par [in,out] status
//! @par [in,out] index
void FilamentLoaded::getNext(uint8_t& status, int16_t& index)
{
    switch(status)
    {
    case KeyFront1:
    case KeyFront2:
        ++index;
        if ((index < 0) || (static_cast<uint16_t>(index) >= ARR_SIZE(eeprom_t::eepromFilament)))
        {
            getNext(status);
            index = ARR_SIZE(eeprom_t::eepromFilament) - 1;
        }
        break;
    case KeyReverse1:
    case KeyReverse2:
        --index;
        if ((index < 0) || (static_cast<uint16_t>(index) >= ARR_SIZE(eeprom_t::eepromFilament)))
        {
            getNext(status);
            index = 0;
        }
        break;
    default:
        status = KeyFront1;
        index = 0;
        break;
    }
}

//! @brief Get next status
//!
//! Sets status to next indexing mode.
//!
//! @par [in,out] status
void FilamentLoaded::getNext(uint8_t& status)
{
    switch(status)
    {
    case KeyFront1:
        status = KeyReverse1;
        break;
    case KeyReverse1:
        status = KeyFront2;
        break;
    case KeyFront2:
        status = KeyReverse2;
        break;
    case KeyReverse2:
        status = KeyFront1;
        break;
    default:
        status = KeyFront1;
        break;
    }
}
