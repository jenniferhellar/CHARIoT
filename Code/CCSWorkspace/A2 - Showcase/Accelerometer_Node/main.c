#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "comms_spi.h"
#include "pwr_control.h"
#include "p7_i2c.h"
#include "util.h"
#include "fuel_gauge.h"
#include "accel.h"

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

#pragma PERSISTENT(setup_count)
#pragma PERSISTENT(flag_data_calculated)
#pragma PERSISTENT(tx_data)
uint8_t setup_count = 0;
uint8_t flag_data_calculated = 0;
char tx_data[] = {'d','a','t','a',':','a','c','c','e','l',':',0,0};

int main(void)
{
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

    if (!setup_count) {
        accel_setup();
        setup_count = 100;
    } else {
        setup_count--;
    }

    if (!flag_data_calculated) {
        // If we need to get accelerometer data, do it now and save to FRAM.

        // START Wait for accelerometer interrupt
        msp_p2_3_setup();
        __bis_SR_register(LPM0_bits + GIE);
        // END Wait for accelerometer interrupt

        // Get battery voltage data:
        volt_data = fuel_gauge_read_voltage();
        tx_data[11] = volt_data & 0x00FF;
        tx_data[12] = (volt_data & 0xFF00) >> 8;
        flag_data_calculated = 1;
    }

    while (1) {
        // Main while loop:

        result = comms_spi_write(tx_data, 13);

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

// Port 2 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2 (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(PORT2_VECTOR))) Port_2 (void)
#else
#error Compiler not supported!
#endif
{
  P2IE  &= ~BIT3;   // Disable interrupt on P2.3
  P2IFG &= ~BIT3;   // No interrupt pending (don't want to trigger it again)
  __bic_SR_register_on_exit(LPM0_bits + GIE);  // Exit low power mode
}
