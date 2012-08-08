#ifndef __LOGGER_H
#define __LOGGER_H

#define LOGGER_BUFFER_SIZE 128


void logger_logString(char* data);
void logger_logStringln(char* data);
void logger_logNumber(uint32_t value);
void logger_logNumberln(uint32_t value);
void logger_logCRLF();
volatile void logger_logByte(uint8_t data);
volatile uint8_t logger_isEmpty();
volatile uint8_t logger_isFull();
volatile uint8_t logger_read();
volatile uint8_t logger_dataAvailable();
volatile uint8_t logger_count();

#endif /* end __LOGGER_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
