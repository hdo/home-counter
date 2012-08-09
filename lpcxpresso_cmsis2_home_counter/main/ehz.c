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

/* we're looking for pattern  "1*255(" */
const uint8_t search_pattern[SEARCH_PATTERN_LENGTH] = {0x31,0x2A,0x32,0x35,0x35,0x28};
uint8_t search_match = 0;
uint8_t serialbuffer[SERIAL_BUFFER_SIZE];
uint8_t serialbuffer_index = 0;

volatile uint8_t value_parsed = 0;
uint32_t ehz_value = 0;
uint32_t ehz_value2 = 0;

uint32_t old_ehz_value = 0;
uint32_t last_ehz_value = 0;
uint32_t last_ehz_msTicks = 0;
uint32_t parsing_errors = 0;

void ehz_process_serial_data(uint8_t data) {
	// convert to 7e1
	data &=0b01111111;
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

					if (last_ehz_value == 0) {
						last_ehz_value = ehz_value;
						last_ehz_msTicks = current_msTicks;
					}
					else {
						uint32_t diff = 0;
						if (current_msTicks > last_ehz_msTicks) {
							diff = current_msTicks - last_ehz_msTicks;
						}
						else {
							// check for timer overflow
							diff = UINT32_MAX - last_ehz_msTicks + current_msTicks;
						}
						logger_logString("diff ticks: ");
						logger_logNumberln(diff);
						uint32_t diffv = ehz_value - last_ehz_value;
						logger_logString("diff value: ");
						logger_logNumberln(diffv);
						// 10 seconds
						if (diff > 1000) {
							ehz_value2 = diffv * 360000 / diff;

							logger_logString("ehz: estimated ehz value2: ");
							logger_logNumberln(ehz_value2);

							last_ehz_msTicks = current_msTicks;
							last_ehz_value = ehz_value;
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
