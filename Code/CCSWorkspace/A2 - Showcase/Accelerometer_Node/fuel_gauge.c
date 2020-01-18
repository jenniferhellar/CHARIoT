#include <msp430.h>
#include <stdint.h>
#include "p7_i2c.h"

#define I2C_ADDR 0x55

/*
 * Define function call addresses:
 */
#define FUNC_CONTROL          0x00
#define FUNC_VOLTAGE          0x04
#define FUNC_SOC              0x1C
#define FUNC_FLAGS            0x06
#define FUNC_BLOCK_DATA_CHKSM 0x60
#define FUNC_BLOCK_DATA_CTL   0x61
#define FUNC_DATA_CLASS       0x3E
#define FUNC_DATA_OFFSET      0x3F

static uint8_t CMD_UNSEAL[] = {FUNC_CONTROL, 0x00, 0x80};
static uint8_t CMD_CFGUPD[] = {FUNC_CONTROL, 0x13, 0x00};
static uint8_t CMD_FLAGS[] = {FUNC_FLAGS};
static uint8_t CMD_EN_BLOCK_DATA_CTL[] = {FUNC_BLOCK_DATA_CTL, 0x00};
static uint8_t CMD_DATA_CLASS[] = {FUNC_DATA_CLASS, 0x52};
static uint8_t CMD_DATA_OFFSET[] = {FUNC_DATA_OFFSET, 0x00};
static uint8_t CMD_DATA_CHKSM[] = {FUNC_BLOCK_DATA_CHKSM};
static uint8_t CMD_READ_OLD_CAP_HIGH[] = {0x46};
static uint8_t CMD_READ_OLD_CAP_LOW[] = {0x47};
static uint8_t CMD_WRITE_CAP_HIGH[] = {0x46, 0x00};
static uint8_t CMD_WRITE_CAP_LOW[] = {0x47, 110};
static uint8_t CMD_READ_OLD_NRG_HIGH[] = {0x48};
static uint8_t CMD_READ_OLD_NRG_LOW[] = {0x49};
static uint8_t CMD_WRITE_NRG_HIGH[] = {0x48, 0x01};
static uint8_t CMD_WRITE_NRG_LOW[] = {0x49, 0x97};
static uint8_t CMD_READ_OLD_VOLT_HIGH[] = {0x4A};
static uint8_t CMD_READ_OLD_VOLT_LOW[] = {0x4B};
static uint8_t CMD_WRITE_VOLT_HIGH[] = {0x4A, 0x0B};
static uint8_t CMD_WRITE_VOLT_LOW[] = {0x4B, 0xB8};
static uint8_t CMD_READ_OLD_TAP_HIGH[] = {0x55};
static uint8_t CMD_READ_OLD_TAP_LOW[] = {0x56};
static uint8_t CMD_WRITE_TAP_HIGH[] = {0x55, 0x00};
static uint8_t CMD_WRITE_TAP_LOW[] = {0x56, 0xDC};
static uint8_t CMD_WRITE_CHKSM[] = {FUNC_BLOCK_DATA_CHKSM, 0x00};
static uint8_t CMD_SOFTRESET[] = {FUNC_CONTROL, 0x42, 0x00};
static uint8_t CMD_SEAL[] = {FUNC_CONTROL, 0x20, 0x00};
static uint8_t CMD_CHEM_B[] = {FUNC_CONTROL, 0x31, 0x00};
static uint8_t CMD_LOAD_CHEM_ID[] = {FUNC_CONTROL, 0x08, 0x00};
static uint8_t CMD_READ_CONTROL[] = {FUNC_CONTROL};

void fuel_gauge_config_update() {
    uint8_t success;
    uint8_t *res;
    uint8_t old_chksm, old_cap_high, old_cap_low, old_nrg_high, old_nrg_low, old_volt_high, old_volt_low, old_tap_high, old_tap_low, temp;

    // Unseal the fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_UNSEAL, 3);
    success = p7_i2c_write(I2C_ADDR, CMD_UNSEAL, 3);
    __delay_cycles(16000000);

    // Set the fuel gauge to config update mode:
    success = p7_i2c_write(I2C_ADDR, CMD_CFGUPD, 3);

    // Confirm that the fuel gauge is in config update mode:
    __delay_cycles(16000000);
    res = p7_i2c_reg_read(I2C_ADDR, CMD_FLAGS, 1, 2);
    while (!((*res) & BIT4)) {
        __delay_cycles(16000000);
        res = p7_i2c_reg_read(I2C_ADDR, CMD_FLAGS, 1, 2);
    }
    /*
    // Update the chemistry profile to CHEM_B:
    success = p7_i2c_write(I2C_ADDR, CMD_CHEM_B, 3);
    __delay_cycles(16000000);
    */
    // Enable block data control for fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_EN_BLOCK_DATA_CTL, 2);

    // Set the data class to be accessed:
    success = p7_i2c_write(I2C_ADDR, CMD_DATA_CLASS, 2);
    __delay_cycles(16000000);

    // Set the data offset to be accessed:
    success = p7_i2c_write(I2C_ADDR, CMD_DATA_OFFSET, 2);

    // Read the current checksum:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_DATA_CHKSM, 1, 1);
    old_chksm = *res;
    __delay_cycles(16000000);

    // Read the current capacity:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_READ_OLD_CAP_HIGH, 1, 2);
    old_cap_high = *res;
    old_cap_low = *(res + 1);

    // Write the new capacity:
    __delay_cycles(16000000);
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_CAP_HIGH, 2);
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_CAP_LOW, 2);

    // Compute the new checksum:
    temp = (255 - old_chksm - old_cap_high - old_cap_low) & 0xFF;
    temp = (temp + CMD_WRITE_CAP_HIGH[1] + CMD_WRITE_CAP_LOW[1]) & 0xFF;
    CMD_WRITE_CHKSM[1] = 255 - temp;
    __delay_cycles(16000000);

    // Write the new checksum to the fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_CHKSM, 2);

    // Read the current checksum:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_DATA_CHKSM, 1, 1);
    old_chksm = *res;
    __delay_cycles(16000000);

    // Read the current energy:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_READ_OLD_NRG_HIGH, 1, 2);
    old_nrg_high = *res;
    old_nrg_low = *(res + 1);
    __delay_cycles(16000000);

    // Write the new energy:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_NRG_HIGH, 2);
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_NRG_LOW, 2);
    __delay_cycles(16000000);

    // Compute the new checksum:
    temp = (255 - old_chksm - old_nrg_high - old_nrg_low) & 0xFF;
    temp = (temp + CMD_WRITE_NRG_HIGH[1] + CMD_WRITE_NRG_LOW[1]) & 0xFF;
    CMD_WRITE_CHKSM[1] = 255 - temp;

    // Write the new checksum to the fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_CHKSM, 2);

    // Read the current checksum:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_DATA_CHKSM, 1, 1);
    old_chksm = *res;
    __delay_cycles(16000000);

    // Read the current term voltage:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_READ_OLD_VOLT_HIGH, 1, 2);
    old_volt_high = *res;
    old_volt_low = *(res + 1);
    __delay_cycles(16000000);

    // Write the new term voltage:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_VOLT_HIGH, 2);
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_VOLT_LOW, 2);
    __delay_cycles(16000000);

    // Compute the new checksum:
    temp = (255 - old_chksm - old_volt_high - old_volt_low) & 0xFF;
    temp = (temp + CMD_WRITE_VOLT_HIGH[1] + CMD_WRITE_VOLT_LOW[1]) & 0xFF;
    CMD_WRITE_CHKSM[1] = 255 - temp;

    // Write the new checksum to the fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_CHKSM, 2);

    // Read the current checksum:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_DATA_CHKSM, 1, 1);
    old_chksm = *res;
    __delay_cycles(16000000);

    // Read the current taper rate:
    res = p7_i2c_reg_read(I2C_ADDR, CMD_READ_OLD_TAP_HIGH, 1, 2);
    old_tap_high = *res;
    old_tap_low = *(res + 1);
    __delay_cycles(16000000);

    // Write the new taper rate:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_TAP_HIGH, 2);
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_TAP_LOW, 2);
    __delay_cycles(16000000);

    // Compute the new checksum:
    temp = (255 - old_chksm - old_tap_high - old_tap_low) & 0xFF;
    temp = (temp + CMD_WRITE_TAP_HIGH[1] + CMD_WRITE_TAP_LOW[1]) & 0xFF;
    CMD_WRITE_CHKSM[1] = 255 - temp;

    // Write the new checksum to the fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_WRITE_CHKSM, 2);

    // Exit CFGUPDATE using a soft reset:
    success = p7_i2c_write(I2C_ADDR, CMD_SOFTRESET, 3);

    // Confirm that the fuel gauge is not in config update mode:
    __delay_cycles(16000000);
    res = p7_i2c_reg_read(I2C_ADDR, CMD_FLAGS, 1, 2);
    while ((*res) & BIT4) {
        __delay_cycles(16000000);
        res = p7_i2c_reg_read(I2C_ADDR, CMD_FLAGS, 1, 2);
    }

    // Re-seal the fuel gauge:
    success = p7_i2c_write(I2C_ADDR, CMD_SEAL, 3);
    __delay_cycles(16000000);
}

static uint8_t CMD_READ_VOLTAGE[] = {FUNC_VOLTAGE};

uint16_t fuel_gauge_read_voltage() {
    uint8_t success;
    uint8_t *res;

    res = p7_i2c_reg_read(I2C_ADDR, CMD_READ_VOLTAGE, 1, 2);

    return (*(res + 1) << 8) + *res;
}

static uint8_t CMD_READ_SOC[] = {FUNC_SOC};

uint16_t fuel_gauge_read_soc() {
    uint8_t success;
    uint8_t *res;

    res = p7_i2c_reg_read(I2C_ADDR, CMD_READ_SOC, 1, 2);

    return (*(res + 1) << 8) + *res;
}
