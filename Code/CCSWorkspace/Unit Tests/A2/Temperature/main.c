#include <msp430.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "temp.h"
#include "p7_i2c.h"

#define SEND_BYTES 4

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
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Per Device Errata set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_4 | DCORSEL;           // Set DCO to 16MHz

    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
    __delay_cycles(60);
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers to 1 for 16MHz operation
    CSCTL0_H = 0;                           // Lock CS registers
}


void main(void)
{
    uint8_t *res_data;
    uint16_t sensor_data[4] = {0};
    uint16_t temp;
    int temp_calculated = 0;
    uint16_t hum_calculated = 0;

    // Disable Watchdog Timer:
    WDTCTL = WDTPW | WDTHOLD;

    // Bump up clock speed:
    initClockTo16MHz();

    // Unlock pins:
    PM5CTL0 &= ~LOCKLPM5;

    // Set debug LED pins to output direction
    P6DIR |= 0x01;
    P6DIR |= 0x02;

    // Turn them off
    P6OUT &= 0x00;

    // Globally enable interrupts:
    __bis_SR_register(GIE);

    // Call the code to initialize each subsystem, in order:
    p7_i2c_setup();

    // Setup 1khz timer A0:
    TA0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
    TA0CCR0 = 4000;
    TA0CTL = TASSEL__ACLK | ID_3 | MC__UP;        // ACLK, UP mode, 32khz/8 = 4khz

    while(1) {
        // Main while loop:
        __bis_SR_register(LPM0_bits + GIE);     // Enter LPM0 w/interrupt (Timer A0 will wake)
        __no_operation();                       // For debugger

        temp_start_measurement();
        P6OUT ^= 0x03;                      // Toggle P6.0 using exclusive-OR (LED1)
        __delay_cycles(250000);

        res_data = temp_read_measurement();
        sensor_data[0] = *res_data;       // 1st temp byte
        sensor_data[1] = *(res_data+1);       // 2nd temp byte
        sensor_data[2] = *(res_data+2);       // 1st humidity byte
        sensor_data[3] = *(res_data+3);       // 2nd humidity byte

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

        __bis_SR_register(LPM0_bits + GIE);
        if((temp_calculated > 0) && (hum_calculated > 0))
            P6OUT &= 0x00;                          // Turn off debug LEDs
    }
}

// Timer0_A0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) Timer0_A0_ISR (void)
#else
#error Compiler not supported!
#endif
 {
     __bic_SR_register_on_exit(LPM0_bits);
 }
