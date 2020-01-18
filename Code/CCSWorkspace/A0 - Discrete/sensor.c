#include <msp430.h>
#include <stdint.h>
#include "sensor.h"
#include "p7_i2c.h"

#define SLAVE_ADDR 0x40
#define TEMP_REG_ADDR 0x00

uint8_t sensor_start_measurement() {
    uint8_t reg_addr;

    // Prepare the register address as a one-byte array:
    reg_addr = TEMP_REG_ADDR;

    // Send a 1-byte message containing the address of the temp register.
    // This signals to the sensor to start measuring.
    return p7_i2c_write(SLAVE_ADDR, &reg_addr, 1, 0);
}

uint8_t *sensor_read_measurement() {
    // Read 4 bytes from the sensor.
    return p7_i2c_read(SLAVE_ADDR, 4);
}
