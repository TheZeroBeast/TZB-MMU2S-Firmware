#ifndef EEPROM_H
#define EEPROM_H
#define E2END 1023u
#include <cstdint>

uint8_t eeprom_read_byte( const uint8_t * __p);
uint16_t eeprom_read_word( const uint16_t * __p);
void eeprom_update_byte( uint8_t * __p, uint8_t __value);
void eeprom_update_word( uint16_t * __p, uint16_t __value);


#endif //EEPROM_H
