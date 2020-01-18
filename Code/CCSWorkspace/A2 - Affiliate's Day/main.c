#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "comms_spi.h"
#include "pwr_control.h"
#include "p7_i2c.h"
#include "temp.h"
#include "util.h"

void initClockTo16MHz() {
    /*
     * Code for this function from TI's I2C standard master example code.
     */

    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz

    // Set SMCLK = MCLK = DCO, ACLK = LFXTCLK (VLOCLK if unavailable)
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Per Device Errata set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_4 | DCORSEL;           // Set DCO to 16MHz

    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
    __delay_cycles(60);
    CSCTL3 = DIVA__1 | DIVS__32 | DIVM__1;
    CSCTL0_H = 0;                           // Lock CS registers
}

static int timer_counter = 0;

#pragma PERSISTENT(flag_data_calculated)
#pragma PERSISTENT(temp_calculated)
#pragma PERSISTENT(hum_calculated)
uint8_t flag_data_calculated = 0;
int temp_calculated = 0;
uint16_t hum_calculated = 0;

static const char *tx_str_template = "Temperature:     C, Relative Humidity:     %";

int main(void)
{
    uint8_t *res_data;
    uint8_t result;
    uint16_t sensor_data[4] = {0};
    uint16_t temp;
    char *tx_str;
    unsigned int i;

    // Disable Watchdog Timer:
    WDTCTL = WDTPW | WDTHOLD;

    // Bump up clock speed:
    initClockTo16MHz();

    // Unlock pins:
    PM5CTL0 &= ~LOCKLPM5;

    // Call the code to initialize each subsystem, in order:
    pwr_control_setup();
    p7_i2c_setup();
    comms_spi_init();   // SPI for comms

    pwr_control_comms(1);

    if (!flag_data_calculated) {
        // If we need to get temp/humidity data, do it now and save to FRAM.
        temp_start_measurement();
        __delay_cycles(250000);

        res_data = temp_read_measurement();
        sensor_data[0] = *res_data;       // 1st temp byte
        sensor_data[1] = *(res_data+1);   // 2nd temp byte
        sensor_data[2] = *(res_data+2);   // 1st humidity byte
        sensor_data[3] = *(res_data+3);   // 2nd humidity byte

        // Conversion process:
        /*
         * To keep this as efficient as possible, we want to avoid using
         * floating-point numbers or divide operations. Also, we are limited
         * to 16-bits of accuracy. So, we need to use some trickery:
         */
        // Multiply both temp bytes by 165deg Celcius
        sensor_data[0] = sensor_data[0] * 165; // 8-bit value becomes 16-bit value
        sensor_data[1] = sensor_data[1] * 165;
        // Multiply both hum bytes by 100 percent
        sensor_data[2] = sensor_data[2] * 100;
        sensor_data[3] = sensor_data[3] * 100;
        // Bit shift upper bytes by 2 to the right, then lower bytes by 10 to the
        // right, then add together, and finally bit shift by 6 more to get the
        // results of temp_bytes*165/2^16 and hum_bytes*100/2^16.
        temp = (((sensor_data[0] >> 2) + (sensor_data[1] >> 10)) >> 6);
        hum_calculated = ((sensor_data[2] >> 2) + (sensor_data[3] >> 10)) >> 6;
        // Also, subtract 40 degrees celcius from the temperature.
        temp_calculated = ((int)temp) - 40;
        flag_data_calculated = 1;
    }

    // Create the string we want to transmit:
    tx_str = malloc(44); //
    for (i = 0; i < 44; i++) {
        tx_str[i] = tx_str_template[i];
    }
    write_int_to_string(temp_calculated, tx_str + 13);
    write_int_to_string(hum_calculated, tx_str + 39);

    while (1) {
        // Main while loop:

        result = comms_spi_write(tx_str, 44);

        TA0R = 0;
        TA0CCR0 = 62500;
        TA0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
        if (result) {
            flag_data_calculated = 0; // data has been transmitted, need to recalculate next time
            timer_counter = 60; // seconds between transmits
        } else {
            timer_counter = 1; // seconds before retry
        }
        TA0CTL = TASSEL__SMCLK | ID_3 | MC__UP; // SMCLK, UP mode, 500kHz/8 = 62.5Hz
        __bis_SR_register(LPM0_bits + GIE);     // Enter LPM0 w/interrupt (Timer A0 will wake)
        __no_operation();
        WDTCTL = 0; // Reset MSP
    }
}

// Timer0_A0 interrupt service routine
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR (void)
 {
    TA0CCTL0 &= ~CCIFG;
    timer_counter--;
    if (timer_counter <= 0) {
        __bic_SR_register_on_exit(LPM0_bits);
        TA0CTL &= ~MC__UP; // Stop timer
        TA0CCTL0 &= ~CCIE;
    }
 }
