#ifndef P7_I2C_H_
#define P7_I2C_H_
#include <stdint.h>

/*
 * Code to initialize the port 7 I2C module.
 */
void p7_i2c_setup();

/*
 * This function writes a series of bytes to the specified address over I2C. If
 * the peripheral device does not ACK a byte, the transmission is aborted and
 * a 0 is returned.
 *
 * Inputs:
 *     addr - A 7-bit peripheral address. NOTICE: address must be right-
 *            justified, e.g. the most-significant bit is ignored.
 *     tx_data - A byte array of data to be transmitted.
 *     len - The number of bytes of data to transmit.
 *
 * Returns:
 *     1 if all of the data was ACKed by the peripheral, a 0 otherwise.
 */
uint8_t p7_i2c_write(uint8_t addr, uint8_t *tx_data, unsigned int len);

/*
 * This function writes a series of bytes, generated a repeated start
 * condition, then reads a series of bytes, to the specified address over I2C.
 * If the peripheral device does not ACK a byte, the transmission is aborted and
 * a 0 is returned.
 *
 * Inputs:
 *     addr - A 7-bit peripheral address. NOTICE: address must be right-
 *            justified, e.g. the most-significant bit is ignored.
 *     tx_data - A byte array of data to be transmitted.
 *     tx_len - The number of bytes of data to transmit.
 *     rx_len - The number of bytes of data to receive.
 *
 * Returns:
 *     A byte array of length data_len received from the peripheral. The memory
 *     for this data is allocated using malloc(rx_len).
 */
uint8_t *p7_i2c_reg_read(uint8_t addr, uint8_t *tx_data, unsigned int tx_len,
                         unsigned int rx_len);

/*
 * This function reads a series of bytes from the specified address over I2C.
 *
 * Inputs:
 *     addr - A 7-bit peripheral address.
 *     len - The number of bytes of data to receive.
 *
 * Returns:
 *     A byte array of length data_len received from the peripheral. The memory
 *     for this data is allocated using malloc(len).
 */
uint8_t *p7_i2c_read(uint8_t addr, unsigned int len);

#endif /* P7_I2C_H_ */
