#ifndef __EEPROM_UTILS_H
#define __EEPROM_UTILS_H

uint32_t eeprom_get_uint32(uint16_t address);
void eeprom_set_uint32(uint16_t address, uint32_t value);
void eeprom_check_update_uint32(uint16_t address, uint32_t value);

#endif /* end __EEPROM_UTILS_H */
