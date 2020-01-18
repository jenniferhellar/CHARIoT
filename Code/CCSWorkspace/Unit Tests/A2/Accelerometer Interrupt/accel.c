/*
 * accel.c
 *
 *  Created on: Apr 6, 2019
 *      Author: jhell
 */

#include "accel.h"

void wait_2us() {
    __delay_cycles(50);
}

void accel_setup() {
    uint8_t success = 0;

    // Disable interrupts on accelerometer
    uint8_t intr_en_0[2] = {BMA2x2_INTR_ENABLE1_ADDR, 0x00};
    uint8_t intr_en_1[2] = {BMA2x2_INTR_ENABLE2_ADDR, 0x00};
    uint8_t intr_en_2[2] = {BMA2x2_INTR_SLOW_NO_MOTION_ADDR, 0x00};

    success = p7_i2c_write(ACCEL_I2C_ADDR, intr_en_0, 2);
    wait_2us();
    success = p7_i2c_write(ACCEL_I2C_ADDR, intr_en_1, 2);
    wait_2us();
    success = p7_i2c_write(ACCEL_I2C_ADDR, intr_en_2, 2);
    wait_2us();

    // Choose parameters
    uint8_t param[2] = {0x00, 0x00};

    /* Acceleration range: +/- 8g (3.91 mg/LSB) */
    uint8_t range = 0x08;
    param[0] = BMA2x2_RANGE_SELECT_ADDR; param[1] = range;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);
    wait_2us();

    /* Interrupt mode: Temporary, 2 sec */
    uint8_t intr_mode = 0x04;
    param[0] = BMA2x2_INTR_CTRL_ADDR; param[1] = intr_mode;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);
    wait_2us();

    /* Interrupt source: Unfiltered data */
    uint8_t intr_src = 0x04;
    param[0] = BMA2x2_INTR_SOURCE_ADDR; param[1] = intr_src;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);
    wait_2us();

    /* Open-drive for INT1, active high */
    uint8_t int1_set = 0x03;
    param[0] = BMA2x2_INTR_SET_ADDR; param[1] = int1_set;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);
    wait_2us();

    /* Slope interrupt mapped to INT1 pin */
    uint8_t intr_map = 0x04;
    param[0] = BMA2x2_INTR1_PAD_SELECT_ADDR; param[1] = intr_map;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);
    wait_2us();

    /* Slope threshold to trigger interrupt */
    uint8_t slope_thres = 0x14;
    param[0] = BMA2x2_SLOPE_THRES_ADDR; param[1] = slope_thres;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);
    wait_2us();

    /* Default: one data point above threshold triggers interrupt */
    uint8_t slope_dur = 0x00;
    param[0] = BMA2x2_SLOPE_DURN_ADDR; param[1] = slope_dur;
    success = p7_i2c_write(ACCEL_I2C_ADDR, param, 2);

    // Wait for 10ms
    __delay_cycles(160000);

    // Re-enable interrupt
    intr_en_0[1] = 0x07;               // All axes enabled
    success = p7_i2c_write(ACCEL_I2C_ADDR, intr_en_0, 2);

    // Configure P2.3 on MSP430 to receive interrupt from accelerometer INT1
    msp_p2_3_setup();
}

void msp_p2_3_setup() {
    // Accelerometer interrupt pin (P2.3) setup:
    P2DIR &= ~BIT3;    // Configure pin as an input
    P2REN |= BIT3;     // Enable pull-up/pull-down resistor
    P2OUT &= ~BIT3;    // Pull-down selected (since interrupt is active high)
    P2IE  |= BIT3;     // Enable interrupt flag for P2IFG3
    P2IES &= ~BIT3;    // P2IFG3 flag set with a low-to-high transition
    P2IFG &= ~BIT3;    // No interrupt pending
}

void accel_off() {
    uint8_t deep_suspend[2] = {BMA2x2_MODE_CTRL_ADDR, 0x20};
    p7_i2c_write(ACCEL_I2C_ADDR, deep_suspend, 2);
    // Perform a soft reset to wake again by calling accel_reset
}

void accel_reset() {
    uint8_t reset[2] = {BMA2x2_RST_ADDR, 0xB6};
    p7_i2c_write(ACCEL_I2C_ADDR, reset, 2);
    // Wait 1.8ms
    __delay_cycles(29000);
}
