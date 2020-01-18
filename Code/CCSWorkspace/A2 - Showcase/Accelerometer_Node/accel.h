#ifndef ACCEL_H_
#define ACCEL_H_

#include <msp430.h>
#include "p7_i2c.h"
#include <stdint.h>
#include <stdlib.h>

#define ACCEL_I2C_ADDR              (0x18)
#define BMA2x2_CHIP_ID_ADDR         (0x00)
/** DATA ADDRESS DEFINITIONS */
#define BMA2x2_X_AXIS_LSB_ADDR      (0x02)
#define BMA2x2_X_AXIS_MSB_ADDR      (0x03)
#define BMA2x2_Y_AXIS_LSB_ADDR      (0x04)
#define BMA2x2_Y_AXIS_MSB_ADDR      (0x05)
#define BMA2x2_Z_AXIS_LSB_ADDR      (0x06)
#define BMA2x2_Z_AXIS_MSB_ADDR      (0x07)
#define BMA2x2_TEMP_ADDR            (0x08)

#define BMA2x2_RANGE_SELECT_ADDR        (0x0F)

/**INTERRUPT ADDRESS DEFINITIONS */
#define BMA2x2_INTR_ENABLE1_ADDR                (0x16)
#define BMA2x2_INTR_ENABLE2_ADDR                (0x17)
#define BMA2x2_INTR_SLOW_NO_MOTION_ADDR         (0x18)
#define BMA2x2_INTR1_PAD_SELECT_ADDR            (0x19)
#define BMA2x2_INTR_DATA_SELECT_ADDR            (0x1A)
#define BMA2x2_INTR2_PAD_SELECT_ADDR            (0x1B)
#define BMA2x2_INTR_SOURCE_ADDR                 (0x1E)
#define BMA2x2_INTR_SET_ADDR                    (0x20)
#define BMA2x2_INTR_CTRL_ADDR                   (0x21)

#define BMA2x2_SLOPE_DURN_ADDR                   (0x27)
#define BMA2x2_SLOPE_THRES_ADDR                  (0x28)

/** Define function prototypes */
void wait_2us();
void accel_setup();
void msp_p2_3_setup();

#endif /* ACCEL_H_ */
