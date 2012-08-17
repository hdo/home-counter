/*
 * ehz.c
 *
 *  Created on: Aug 2, 2012
 *      Author: huy
 */

#include "LPC17xx.h"
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
		sensor_data[i].errors = 0;
		sensor_data[i].name = "-";
	}
}

void add_ehz(uint8_t addr, char* name) {
	sensor_data[addr].enabled = 1;
	sensor_data[addr].type = SENSOR_TYPE_EHZ;
	sensor_data[addr].name = name;
}

void add_s0(uint8_t addr, char* name) {
	sensor_data[addr].enabled = 1;
	sensor_data[addr].type = SENSOR_TYPE_S0;
	sensor_data[addr].name = name;
}

SENSOR_DATA* get_sensor_by_id(uint8_t id) {
	if (id < MAX_SENSORS) {
		return &sensor_data[id];
	}
	else {
		return 0;
	}
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

char* get_sensor_type(uint8_t type) {
	switch(type) {
	case SENSOR_TYPE_NONE : return "NONE";
	case SENSOR_TYPE_EHZ : return "EHZ";
	case SENSOR_TYPE_S0 : return "S0";
	case SENSOR_TYPE_MBUS : return "MBUS";
	}
	return "UNDEFINED";
}


