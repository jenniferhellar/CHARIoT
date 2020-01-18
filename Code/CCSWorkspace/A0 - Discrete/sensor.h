#ifndef SENSOR_H_
#define SENSOR_H_

/*
 * Send an I2C message to the sensor and ask it to start measuring.
 */
uint8_t sensor_start_measurement();

/*
 * Reads the result of a temp/humidity measurement from the sensor.
 * Must be called ~12.8ms after a call to sensor_start_measurement() is
 * finished (see the datasheet).
 *
 * Returns:
 *     A byte array of data (4 bytes long) received from the sensor.
 */
uint8_t *sensor_read_measurement();

#endif /* SENSOR_H_ */
