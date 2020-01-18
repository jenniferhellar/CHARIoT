#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "comms_spi.h"
#include "pwr_control.h"
#include "p7_i2c.h"
#include "temp.h"
#include "util.h"
#include "fuel_gauge.h"

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
#pragma PERSISTENT(tx_data)
uint8_t flag_data_calculated = 0;
char tx_data[16] = {'d','a','t','a',':','t','e','m','p',':',0,0,0,0,0,0};

int main(void)
{
    uint8_t *res_data;
    uint16_t volt_data;
    uint8_t result;

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
        tx_data[10] = *res_data;       // 1st temp byte
        tx_data[11] = *(res_data+1);   // 2nd temp byte
        tx_data[12] = *(res_data+2);   // 1st humidity byte
        tx_data[13] = *(res_data+3);   // 2nd humidity byte

        // Get battery voltage data:
        volt_data = fuel_gauge_read_voltage();
        tx_data[14] = volt_data & 0x00FF;
        tx_data[15] = (volt_data & 0xFF00) >> 8;
        flag_data_calculated = 1;
    }

    while (1) {
        // Main while loop:

        result = comms_spi_write(tx_data, 16);

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
