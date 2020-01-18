#ifndef FUEL_GAUGE_H_
#define FUEL_GAUGE_H_

/*
 * Performs the required config updates to the fuel gauge over the I2C
 * interface.
 *
 * NOT INTENDED FOR USE IN END-APPLICATION CODE.
 */
void fuel_gauge_config_update();

/*
 * Reads the current voltage of the battery using the fuel gauge I2C interface.
 *
 * Returns:
 *     A two-byte value representing the voltage in mV.
 */
uint16_t fuel_gauge_read_voltage();

/*
 * Reads the current state of charge of the battery using the fuel gauge
 * I2C interface.
 *
 * Returns:
 *     A two-byte value representing the voltage in mV.
 */
uint16_t fuel_gauge_read_soc();

#endif /* FUEL_GAUGE_H_ */
