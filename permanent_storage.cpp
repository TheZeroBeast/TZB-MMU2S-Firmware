/**
 * @file
 * @author Marek Bel
 */

#include "permanent_storage.h"
#include "mmctl.h"
#include <avr/eeprom.h>

typedef struct
{
	uint8_t eepromLengthCorrection; //!< legacy
	uint16_t eepromBowdenLen[5];
}eeprom_t;

static eeprom_t * const eepromBase = reinterpret_cast<eeprom_t*>(0);
static const uint16_t eepromEmpty = 0xffff;
static const uint16_t eepromLengthCorrectionBase = 7900u; //!< legacy
static const uint16_t eepromBowdenLenDefault = 8900u;
static const uint16_t eepromBowdenLenMinimum = 6900u;
static const uint16_t eepromBowdenLenMaximum = 10900u;


static bool validFilament(uint8_t filament)
{
	if (filament < (sizeof(eeprom_t::eepromBowdenLen)/sizeof(eeprom_t::eepromBowdenLen[0]))) return true;
	else return false;
}

static bool validBowdenLen (const uint16_t BowdenLength)
{
	if ((BowdenLength >= eepromBowdenLenMinimum)
			&& BowdenLength <= eepromBowdenLenMaximum) return true;
	return false;
}

uint16_t BowdenLength::get()
{
	uint8_t filament = active_extruder;
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




BowdenLength::BowdenLength() : m_filament(active_extruder), m_length(BowdenLength::get())
{
}

bool BowdenLength::increase()
{
	if ( validBowdenLen(m_length + stepSize))
	{
		m_length += stepSize;
		return true;
	}
	return false;
}

bool BowdenLength::decrease()
{
	if ( validBowdenLen(m_length - stepSize))
	{
		m_length -= stepSize;
		return true;
	}
	return false;
}

BowdenLength::~BowdenLength()
{
	if (validFilament(m_filament))eeprom_update_word(&(eepromBase->eepromBowdenLen[m_filament]), m_length);
}

void BowdenLength::eraseAll()
{
	for (uint16_t i = 0; i < 1024; i++)
	{
		eeprom_update_byte((uint8_t*)i, static_cast<uint8_t>(eepromEmpty));
	}
}
