#ifndef __EHZ_H
#define __EHZ_H

#define SERIAL_BUFFER_SIZE 12
#define SEARCH_PATTERN_LENGTH 6
#define EHZ_VALUE_LENGTH 11
#define EHZ_EXPECTED_DIGITS 10


void ehz_process_serial_data(uint8_t data);

#endif /* end __EHZ_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
