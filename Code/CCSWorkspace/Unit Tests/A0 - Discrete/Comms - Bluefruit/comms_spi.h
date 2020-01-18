#ifndef COMMS_SPI_H_
#define COMMS_SPI_H_

#define COMMS_SDEP_MAX_PAYLOAD 16
#define COMMS_SDEP_CMD_INIT    0xBEEF
#define COMMS_SDEP_CMD_AT      0x0A00
#define COMMS_SDEP_CMD_TX      0x0A01
#define COMMS_SDEP_CMD_RX      0x0A02

/*
 * The following two defines determines whether or not the spi code will use
 * software SPI (bit-banging) or hardware SPI (eUSCI module).
 */
//#define COMMS_SPI_BITBANG
#define COMMS_SPI_EUSCI

/*
 * This function sends multiple SDEP frame containing the entire msg to the
 * bluefruit over SPI.
 *
 * Parameters:
 *     cmd_type - One of the 16-bit COMMS_SDEP_CMD_* macros.
 *     msg - A null-terminated msg string.
 */
void comms_sdep_msg(uint16_t cmd_type, char *msg);

/*
 * This function sends a whole SDEP frame to the bluefruit over SPI.
 *
 * Parameters:
 *     cmd_type - One of the 16-bit COMMS_SDEP_CMD_* macros.
 *     more_data - True if there is more data being sent after this frame
 *                 for the same packet (since max frame size is 20 bytes).
 *     payload_size - A number in the range [0, 16] specifying the number of
 *                    bytes in the payload.
 *     payload - An array pointer to the payload to be sent. If payload_size
 *               is 0, this is not touched.
 */
void comms_sdep_packet(uint16_t cmd_type, uint8_t more_data,
                       uint8_t payload_size, uint8_t *payload);

/*
 * Receive an incoming SDEP frame from the bluefruit over SPI.
 *
 * Return:
 *     A pointer to data received (max 20 bytes).
 */
uint8_t *comms_get_sdep_packet();

/*
 * This function pulls the RST pin to ground to reset the Bluefruit for 10us.
 */
void comms_reset();

/*
 * This function initializes the comms SPI code.
 */
void init_comms_spi();
/*
 * This function sends a single byte over SPI to the comms system.
 *
 * Paramters:
 *     b - The byte to send.
 * Returns:
 *     The byte of data received from the peripheral while transmitting.
 */
uint8_t comms_spi_byte(uint8_t b);

#endif /* COMMS_SPI_H_ */
