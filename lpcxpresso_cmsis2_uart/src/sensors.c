/*
 * ehz.c
 *
 *  Created on: Aug 2, 2012
 *      Author: huy
 */

#include "LPC17xx.h"
#include "type.h"
#include "sensors.h"

SENSOR_DATA sensor_data[MAX_SENSORS];

void init_sensors() {
	uint8_t i=0;
	for (; i < MAX_SENSORS; i++) {
		sensor_data[i].id = i;
		sensor_data[i].address = 0;
		sensor_data[i].enabled = 0;
		sensor_data[i].type = SENSOR_TYPE_NONE;
		sensor_data[i].value = 0;
		sensor_data[i].value2 = 0;
	}
}

void add_ehz(uint8_t addr) {
	sensor_data[addr].enabled = 1;
	sensor_data[addr].type = SENSOR_TYPE_EHZ;
}

SENSOR_DATA* get_sensor(uint8_t type, uint8_t addr) {
	uint8_t i=0;
	for(; i < MAX_SENSORS; i++) {
		if (sensor_data[i].type == type && sensor_data[i].address == addr) {
			return &sensor_data[i];
		}
	}
	return 0;
}


