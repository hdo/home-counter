#ifndef __QUEUE_H
#define __QUEUE_H

#define QUEUE_BUFFER_SIZE 256

void   queue_reset();
void   queue_put(uint8_t data);
uint8_t queue_isEmpty();
uint8_t queue_isFull();
uint8_t queue_read();
uint8_t queue_dataAvailable();
uint8_t queue_count();


#endif /* end __QUEUE_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
