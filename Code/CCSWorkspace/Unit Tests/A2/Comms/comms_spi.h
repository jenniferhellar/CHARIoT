#include <stdint.h>

#ifndef COMMS_SPI_H_
#define COMMS_SPI_H_

// Define which board revision to use:
#define COMMS_SPI_REV_2
//#define COMMS_SPI_REV_3

/*
 * Function definitions:
 */
void comms_spi_init();
uint8_t comms_spi_write(uint8_t *data, uint16_t len);
uint8_t *comms_spi_read(int *response_len);

#endif /* COMMS_SPI_H_ */
