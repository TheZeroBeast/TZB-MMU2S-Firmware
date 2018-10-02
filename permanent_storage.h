//! @file
//! @author Marek Bel

#ifndef PERMANENT_STORAGE_H_
#define PERMANENT_STORAGE_H_

#include <stdint.h>

class BowdenLength
{
public:
	BowdenLength();
	bool increase();
	bool decrease();
	~BowdenLength();
	static uint16_t get();
	static const uint8_t stepSize = 10u;
	static void eraseAll();
private:
	uint8_t m_filament;
	uint16_t m_length;
};




#endif /* PERMANENT_STORAGE_H_ */
