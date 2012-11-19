#include "LPC17xx.h"
#include "eeprom_utils.h"
#include "i2c.h"

#define MC24LC64_ADDR 0xa0		/* I2C address of the 24LC64 EEPROM */

uint32_t eeprom_get_uint32(uint16_t address) {

	uint32_t i;

	/* initialize I2c */
	if (I2CInit((uint32_t) I2CMASTER) == 0)	{
		return 1; /* Fatal error */
	}

	for (i = 0; i < I2C_BUFSIZE; i++) {
		I2CSlaveBuffer[i] = 0x00;
	}

	/* Write SLA(W), address, SLA(R), and read 4 bytes back. */

	I2CWriteLength = 3;
	I2CReadLength = 4;
	I2CMasterBuffer[0] = MC24LC64_ADDR;
	I2CMasterBuffer[1] = (address >> 8) & 0xFF; /* address MSB */
	I2CMasterBuffer[2] = address & 0xFF;        /* address LSB */
	I2CMasterBuffer[3] = MC24LC64_ADDR | RD_BIT;

	// block until read is done
	while (I2CEngine() == I2CSTATE_SLA_NACK);

	i = 0;
	i |= I2CSlaveBuffer[0];
	i = i << 8;
	i |= I2CSlaveBuffer[1];
	i = i << 8;
	i |= I2CSlaveBuffer[2];
	i = i << 8;
	i |= I2CSlaveBuffer[3];
	return i;
}

void eeprom_set_uint32(uint16_t address, uint32_t value) {

	/* initialize I2c */
	if (I2CInit((uint32_t) I2CMASTER) == 0) {
		return; /* Fatal error */
	}
	/* Write SLA(W), address and one data byte */
	I2CWriteLength = 7;
	I2CReadLength = 0;
	I2CMasterBuffer[0] = MC24LC64_ADDR;
	I2CMasterBuffer[1] = (address >> 8) & 0xFF; /* address MSB */
	I2CMasterBuffer[2] = address & 0xFF;        /* address LSB */

	I2CMasterBuffer[3] = (value >> 24) & 0xFF; /* MSB */
	I2CMasterBuffer[4] = (value >> 16) & 0xFF;
	I2CMasterBuffer[5] = (value >> 8) & 0xFF;
	I2CMasterBuffer[6] = value & 0xFF;         /* LSB */
	I2CEngine();
	return;
}

void eeprom_check_update_uint32(uint16_t address, uint32_t value) {
	uint32_t temp = eeprom_get_uint32(address);
	if (temp != value) {
		eeprom_set_uint32(address, value);
	}
}
