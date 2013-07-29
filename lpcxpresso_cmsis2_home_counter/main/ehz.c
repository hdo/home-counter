/*
 * ehz.c
 *
 *  Created on: 2013-07-29
 *      Author: huy
 */

#include "LPC17xx.h"
#include "ehz.h"
#include "logger.h"
#include "math_utils.h"


/* we're looking for pattern  "1*255(" */
uint8_t search_pattern_current_power[]   = { 0x77, 0x07, 0x01, 0x00, 0x10, 0x07, 0x00, 0xFF };
uint8_t search_pattern_energy_produced[] = { 0x77, 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xFF };
uint8_t search_pattern_energy_consumed[] = { 0x77, 0x07, 0x01, 0x00, 0x02, 0x08, 0x00, 0xFF };

uint32_t ehz_power_current_consume_value = 0;
uint32_t ehz_power_current_produce_value = 0;
uint32_t ehz_energy_produced_value = 0;
uint32_t ehz_energy_consumed_value = 0;


uint8_t serialbuffer[EHZ_SERIAL_BUFFER_SIZE];
uint16_t serialbuffer_index = 0;
uint16_t current_index = 0;
uint32_t current_msticks = 0;
uint32_t last_receive_msticks = 0;

uint8_t data_available = 0;
uint8_t error_available = 0;

void ehz_reset() {
	uint16_t i=0;
	for(;i < EHZ_SERIAL_BUFFER_SIZE; i++) {
		serialbuffer[i] = 0;
	}
	data_available = 0;
	error_available = 0;
}

void ehz_init() {
	ehz_reset();
}

uint8_t is_match_id(uint8_t match_id[]) {
	if (current_index + sizeof(match_id) >= serialbuffer_index)  {
		return 0;
	}
	uint8_t i=0;
	uint8_t matched = 1;
	for(; i < sizeof(match_id) && matched; i++) {
		if (serialbuffer[current_index+i] != match_id[i]) {
			matched = 0;
		}
	}
	return matched;
}


void ehz_process_serial_data(uint8_t data) {
	last_receive_msticks = current_msticks;
	if (serialbuffer_index < EHZ_SERIAL_BUFFER_SIZE) {
		serialbuffer[serialbuffer_index++] = data;
	}
}


void ehz_process(uint32_t msticks) {
	current_msticks = msticks;

	// wait until 50ms of uart idle
	if (math_calc_diff(current_msticks, last_receive_msticks) > 5 && serialbuffer_index > 20) {
		// parse data

		logger_logStringln("parsing ehz data ...");

		current_index = 0;
		data_available = 0;
		error_available = 0;
		while(current_index < serialbuffer_index) {
			// skip until first identifier (0x77)
			while(serialbuffer[current_index++] != 0x77 && current_index < serialbuffer_index);

			if (current_index >= serialbuffer_index) {
				break;
			}

			if (current_index + 24 > serialbuffer_index) {
				continue;
			}

			if (is_match_id(search_pattern_current_power)) {
				// read value
				if (serialbuffer[current_index+14] == 0x55 && serialbuffer[current_index+19] == 0x01) {
					int32_t v = (serialbuffer[current_index+15] << 24)
							| (serialbuffer[current_index+16] << 16)
							| (serialbuffer[current_index+17] << 8)
							| (serialbuffer[current_index+18]);
					if (v > 0) {
						ehz_power_current_consume_value = v;
						logger_logString("current power consume: ");
						logger_logNumberln(ehz_power_current_consume_value);
					}
					else {
						ehz_power_current_produce_value = (-1*v);
						logger_logString("current power produce: ");
						logger_logNumberln(ehz_power_current_produce_value);
					}
				}
				else {
					logger_logStringln("Error parsing current power!");
					error_available++;
				}
				continue;
			}

			if (is_match_id(search_pattern_energy_produced)) {
				// read value
				if (serialbuffer[current_index+17] == 0x56 && serialbuffer[current_index+23] == 0x01) {
					uint32_t v = (serialbuffer[current_index+19] << 24)
							| (serialbuffer[current_index+20] << 16)
							| (serialbuffer[current_index+21] << 8)
							| (serialbuffer[current_index+22]);
					ehz_energy_produced_value = v;
					logger_logString("energy produced: ");
					logger_logNumberln(ehz_energy_produced_value);
				}
				else {
					logger_logStringln("Error parsing current power!");
					error_available++;
				}
				continue;
			}

			if (is_match_id(search_pattern_energy_consumed)) {
				// read value
				if (serialbuffer[current_index+17] == 0x56 && serialbuffer[current_index+23] == 0x01) {
					uint32_t v = (serialbuffer[current_index+19] << 24)
							| (serialbuffer[current_index+20] << 16)
							| (serialbuffer[current_index+21] << 8)
							| (serialbuffer[current_index+22]);
					ehz_energy_consumed_value = v;
					logger_logString("energy consumed: ");
					logger_logNumberln(ehz_energy_consumed_value);
				}
				else {
					logger_logStringln("Error parsing current power!");
					error_available++;
				}
				continue;
			}
		}
		if (error_available == 0) {
			data_available = 1;
		}
	}
}

uint8_t ehz_data_available() {
	return data_available;
}

uint8_t ehz_error_available() {
	return error_available;
}

uint32_t ehz_get_power_current_produce() {
	return ehz_power_current_consume_value;
}

uint32_t ehz_get_power_current_consume() {
	return ehz_power_current_produce_value;
}

uint32_t ehz_get_energy_produced() {
	return ehz_energy_produced_value;
}

uint32_t ehz_get_energy_consumed() {
	return ehz_energy_consumed_value;
}



