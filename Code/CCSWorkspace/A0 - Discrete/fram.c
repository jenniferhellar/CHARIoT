#include <msp430.h>
#include <stdint.h>
#include "sensor.h"
#include "p7_i2c.h"
#include "fram.h"

#pragma PERSISTENT(fram_arr)
#pragma PERSISTENT(bytes_stored)

uint8_t bytes_stored = 0;
uint8_t fram_arr[FRAM_SIZE] = {0};
uint8_t *curr = &fram_arr[0];

void fram_write(uint8_t *data_ptr, int numbytes) {
    int i;
    for (i=0; i<numbytes; i++) {
        *curr = *data_ptr;
        curr++;
        data_ptr++;
        if (curr-fram_arr > FRAM_SIZE){
            curr = &fram_arr[0];        // loop back to start
        } else {
            bytes_stored++;
        }
    }
}

