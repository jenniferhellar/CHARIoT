#include <stdint.h>

#ifndef COMMS_H_
#define COMMS_H_

#define COMMS_SDEP_MAX_PAYLOAD    16
#define COMMS_SDEP_CMD_INIT       0xBEEF
#define COMMS_SDEP_CMD_AT         0x0A00
#define COMMS_SDEP_CMD_TX         0x0A01
#define COMMS_SDEP_CMD_RX         0x0A02
#define SDEP_MSGTYPE_CMD          0x10
#define SDEP_MSGTYPE_RESPONSE     0x20
#define SDEP_MSGTYPE_ALERT        0x40
#define SDEP_MSGTYPE_ERROR        0x80
#define SDEP_MSGTYPE_GETRESPONSE  0xFF
#define SPI_IGNORED_BYTE          0xFE
#define SPI_OVERREAD_BYTE         0xFF

/*
 * Code to initialize the comms system.
 */
void comms_setup();

/*
 * This function sends a single byte over SPI to the comms system.
 *
 * Paramters:
 *     b - The byte to send.
 *     wait_for_response - Whether or not the function should wait for
 *                         the TX/RX cycle to finish and return what the
 *                         the comms responded or not.
 * Returns:
 *     The byte of data received from the peripheral while transmitting,
 *     if wait_for_response is non-zero, otherwise 0.
 */
uint8_t comms_spi_byte(uint8_t b, uint8_t wait_for_response);

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
 * This function checks whether or not the IRQ line is being pulled by the
 * Bluefruit, and if so, contacts the Bluefruit to get the pending packet.
 *
 * WARNING: This function uses the same data memory location to store the
 * response for every function call. The response should be fully processed
 * or copied before this function is called again.
 *
 * Returns:
 *     A pointer to a max 20-byte string (21 with NUL byte at end) containing
 *     the response from the Bluefruit.
 *
 *     The first byte contains what type of packet it is (SDEP_MSGTYPE_RESPONSE,
 *     SDEP_MSGTYPE_ALERT or SDEP_MSGTYPE_ERROR).
 *
 *     The next two bytes contain the command ID of the packet (probably one of
 *     the COMMS_SDEP_CMD_* values), in little endian format.
 *
 *     The third byte contains the length of the response (in the range [0, 15])
 *     in the right-most (least significant) 5 bits, and the left-most (most
 *     significant) bit is 1 if the response is not finished yet, and another
 *     packet should be pending with more of the response.
 *
 *     Finally, the remaining 16 (or less) bytes contain the packet data,
 *     usually in ASCII format. The string is guaranteed to be NUL-terminated.
 *
 */
char *comms_get_sdep_packet();

/*
 * This function sends multiple SDEP frame containing the entire msg to the
 * bluefruit over SPI.
 *
 * Parameters:
 *     cmd_type - One of the 16-bit COMMS_SDEP_CMD_* macros.
 *     msg - A null-terminated msg string.
 */
void comms_sdep_msg(uint16_t cmd_type, char *msg);

#endif /* COMMS_H_ */
