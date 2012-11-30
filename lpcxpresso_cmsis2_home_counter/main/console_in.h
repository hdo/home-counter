#ifndef __CONSOLE_IN_H
#define __CONSOLE_IN_H

#define CONSOLE_IN_BUFFER_SIZE 255

void   console_in_reset();
void   console_in_put(uint8_t data);
uint8_t console_in_isEmpty();
uint8_t console_in_isFull();
uint8_t console_in_read();
uint8_t console_in_dataAvailable();
uint8_t console_in_count();


#endif /* end __CONSOLE_IN_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
