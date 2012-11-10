/*
 * ehz.c
 *
 *  Created on: Aug 2, 2012
 *      Author: huy
 */

#include "LPC17xx.h"
#include "uart.h"
#include "ehz.h"
#include "logger.h"
#include "clock-arch.h"

#define TICK_MS 10 // each tick equals 10 ms (see clock-arch.h)
#define UPDATE_ESTIMATE_SECONDS 5 // update estimated value each x seconds (e.g. 5)
#define UPDATE_ESTIMATE_CANDIDATE_DIFF 20 // last candidate should be from x seconds in the past (e.g. 20)
#define TICKS_PER_OUR 1000 * 3600 / TICK_MS

// there is no need to change the following formula
#define UPDATE_TRIGGER UPDATE_ESTIMATE_SECONDS * 1000 / TICK_MS
#define CANDIDATE_TRIGGER UPDATE_ESTIMATE_CANDIDATE_DIFF * 1000 / TICK_MS


#define QUEUE_SIZE 6

/* we're looking for pattern  "1*255(" */
const uint8_t search_pattern[SEARCH_PATTERN_LENGTH] = {0x31,0x2A,0x32,0x35,0x35,0x28};
uint8_t search_match = 0;
uint8_t serialbuffer[SERIAL_BUFFER_SIZE];
uint8_t serialbuffer_index = 0;

uint8_t value_parsed = 0;
uint32_t ehz_value = 0;
uint32_t ehz_value2 = 0;

uint32_t old_ehz_value = 0;
uint32_t last_ehz_msTicks = 0;
uint32_t parsing_errors = 0;

uint8_t queue_index = 0;
uint32_t queue_values[QUEUE_SIZE];
uint32_t queue_msticks[QUEUE_SIZE];

void ehz_init() {
	uint8_t i=0;
	queue_index = 0;
	for(;i < QUEUE_SIZE; i++) {
		queue_values[i] = 0;
		queue_msticks[i] = 0;
	}
}

void add_to_queue(uint32_t msticks, uint32_t value) {
	queue_msticks[queue_index] = msticks;
	queue_values[queue_index] = value;
	queue_index++;
	queue_index %= QUEUE_SIZE;
}

uint32_t get_diff(uint32_t value1, uint32_t value2) {
	if (value1 == value2) {
		return 0;
	}
	if (value1 > value2) {
		return (value1 - value2);
	}
	else {
		// check for timer overflow
		return (UINT32_MAX - value2 + value1);
	}
}

int8_t get_index_for_calculation(uint32_t msticks) {
	uint8_t i=0;
	int8_t candidate_index = -1;
	uint32_t candidate_d = 0;
	for(;i < QUEUE_SIZE; i++) {
		if (queue_msticks[i] == 0) {
			continue;
		}
		uint32_t d = get_diff(msticks, queue_msticks[i]);
		// 20 seconds
		if (d >= CANDIDATE_TRIGGER) {
			// if there isn't any candidate yet
			if (candidate_d == 0) {
				candidate_d = d;
				candidate_index = i;
			}
			// if there is alread a candidate
			// find a better one
			else {
				if (d < candidate_d) {
					candidate_d = d;
					candidate_index = i;
				}
			}
		}
	}
	return candidate_index;
}


void ehz_process_serial_data(uint8_t data) {
	// convert to 7e1
	data &=0b01111111;
	// echo
	// logger_logByte(data);
	if (search_match >= SEARCH_PATTERN_LENGTH) {

		// here comes the data
		if (serialbuffer_index >= SERIAL_BUFFER_SIZE) {
			// error (should not happen)
			serialbuffer_index = 0;
			search_match = 0;
			logger_logStringln("ehz: unexpected buffer overflow");
		}
		serialbuffer[serialbuffer_index++] = data;
		if (serialbuffer_index >= EHZ_VALUE_LENGTH || data == ')') {

			// we're expecting 11 bytes of data
			// * parse data here *
			uint8_t i = 0;
			uint8_t d;
			// atoi conversion, ignoring non-digits
			ehz_value = 0;
			uint8_t digits = 0;
			uint8_t decPosition = 0;
			for (;i<serialbuffer_index;i++) {
				d = serialbuffer[i];
				if (d >= '0' && d <= '9') {
					digits++;
					d -= '0';
					ehz_value *= 10;
					ehz_value += d;
				}
				if (d == '.') {
					decPosition = i;
				}
			}

			// reset buffer
			serialbuffer_index = 0;
			search_match = 0;

			// we're expecting 10 digits for correctly parsed values
			// e.g.
			// 1*255(008433.1524)
			// 1*255(008433.1531)
			// 1*255(008433.1614)
			if (digits == EHZ_EXPECTED_DIGITS && decPosition == EHZ_EXPECTED_DECIMAL_POSITION) {
				value_parsed = 1;

				if (old_ehz_value == 0) {
					old_ehz_value = ehz_value;
				}
				if (ehz_value >= old_ehz_value) {
					old_ehz_value = ehz_value;
					uint32_t current_msTicks = clock_time(); // one tick equals 10ms see lpc17xx_systick.h
					logger_logString("main: ehz value: ");
					logger_logNumberln(ehz_value);

					if (last_ehz_msTicks == 0) {
						last_ehz_msTicks = current_msTicks;
						add_to_queue(current_msTicks, ehz_value);
					}
					else {
						uint32_t diff = get_diff(current_msTicks, last_ehz_msTicks);
						logger_logString("diff ticks: ");
						logger_logNumberln(diff);

						// do a calculation each 5 seconds
						// 5 seconds
						if (diff >= UPDATE_TRIGGER) {

							int8_t candidate_index = get_index_for_calculation(current_msTicks);
							if (candidate_index > -1) {
								logger_logString("candidate index: ");
								logger_logNumberln(candidate_index);
								uint32_t prev_msticks = queue_msticks[candidate_index];
								uint32_t prev_value = queue_values[candidate_index];
								uint32_t d_ticks = get_diff(current_msTicks, prev_msticks);
								uint32_t d_value = get_diff(ehz_value, prev_value);
								logger_logString("candidate mstick diff: ");
								logger_logNumberln(d_ticks);
								logger_logString("candidate value diff: ");
								logger_logNumberln(d_value);
								ehz_value2 = (d_value * TICKS_PER_OUR) / d_ticks;

								logger_logString("ehz: estimated ehz value2: ");
								logger_logNumberln(ehz_value2);
							}
							else {
								logger_logStringln("ehz: no candidate");
							}

							add_to_queue(current_msTicks, ehz_value);
							last_ehz_msTicks = current_msTicks;
						}
					}
				}
				else {
					// handle unexpected parsing error
					// log error
					// value is expected to be greater than previous value
					parsing_errors++;
					logger_logString("ehz: error count: ");
					logger_logNumberln(parsing_errors);
					logger_logString("ehz: unexpected ehz value: ");
					logger_logNumber(ehz_value);
					logger_logString(" old value: ");
					logger_logNumberln(old_ehz_value);
				}
			}
			else {
				// log error
				logger_logString("ehz: parsing error digits: ");
				logger_logNumberln(digits);
				logger_logString("ehz: parsing error decimal position: ");
				logger_logNumberln(decPosition);
			}
		}
	}
	else {
		if (data == search_pattern[search_match]) {
			search_match++;
			if (search_match == SEARCH_PATTERN_LENGTH) {
				logger_logStringln("ehz: triggered");
			}
		}
		else {
			search_match = 0;
		}
	}
}

uint8_t ehz_value_parsed() {
	return value_parsed;
}

uint32_t ehz_get_value() {
	value_parsed = 0;
	return ehz_value;
}

uint32_t ehz_get_estimated_value() {
	return ehz_value2;
}

uint32_t ehz_get_parsing_errors() {
	return parsing_errors;
}
