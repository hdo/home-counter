#ifndef __SENSORS_H
#define __SENSORS_H

#define MAX_SENSORS 8
#define SENSOR_TYPE_NONE 0
#define SENSOR_TYPE_EHZ  1
#define SENSOR_TYPE_S0   2
#define SENSOR_TYPE_MBUS 3


typedef struct sensordata {
    uint8_t id;
    uint8_t enabled;
    uint8_t type; /* ehz, s0, mbus */
    uint8_t address;
    uint32_t value;
    uint32_t value2;
} SENSOR_DATA;

void init_sensors();

void add_ehz(uint8_t addr);
SENSOR_DATA* get_sensor(uint8_t type, uint8_t addr);

#endif /* end __SENSORS_H */


