#ifndef __EHZ_H
#define __EHZ_H

#define EHZ_SERIAL_BUFFER_SIZE 512

void ehz_reset();
void ehz_init();
void ehz_process(uint32_t msticks);
void ehz_process_serial_data(uint8_t data);

uint8_t ehz_data_available();
uint8_t ehz_error_available();

uint32_t ehz_get_power_current_produce();
uint32_t ehz_get_power_current_consume();
uint32_t ehz_get_energy_produced();
uint32_t ehz_get_energy_consumed();


#endif /* end __EHZ_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
