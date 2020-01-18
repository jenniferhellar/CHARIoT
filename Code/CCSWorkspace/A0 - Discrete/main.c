#include <msp430.h> 
#include <stdio.h>
#include <stdlib.h>
#include "pwr_control.h"
#include "comms.h"
#include "sensor.h"
#include "p7_i2c.h"
#include "fram.h"
#include "fuel_gauge.h"

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

static char num_str[3] = {0};
char *short_to_str(uint16_t data) {
    num_str[0] = (data & 0xFF00) >> 8;
    num_str[1] = data & 0x00FF;
    return num_str;
}

void main(void)
{
    uint8_t *res_data;
    uint16_t res;

    // Disable Watchdog Timer:
    WDTCTL = WDTPW | WDTHOLD;

    // Bump up clock speed:
    initClockTo16MHz();

    // Unlock pins:
    PM5CTL0 &= ~LOCKLPM5;

    // Globally enable interrupts:
    __bis_SR_register(GIE);

    // Call the code to initialize each subsystem, in order:
    p7_i2c_setup();
    pwr_control_setup();
    comms_setup();

//    uint8_t bytes_sent = 0;
//    while (bytes_stored > 0) {
//        comms_sdep_packet(COMMS_SDEP_CMD_TX, 0, 4, &fram_arr[0]);
//        bytes_sent += SEND_BYTES;
//        __delay_cycles(16000000);
//    }

    // Setup 1khz timer A0:
    TA0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
    TA0CCR0 = 4000;
    TA0CTL = TASSEL__ACLK | ID_3 | MC__UP;        // ACLK, UP mode, 32khz/8 = 4khz
//    int i;
    uint8_t volt_data[6] = {0};
    int first = 1;
//    uint16_t thresh = 3.1;

    while(1) {
        // Main while loop:
        __bis_SR_register(LPM0_bits + GIE);     // Enter LPM0 w/ interrupt
        __no_operation();                       // For debugger

        // Do stuff here:
        res = fuel_gauge_read_voltage();
        if ((float) res > 3.1) {
            volt_data[0] = res & 0xFF;
            volt_data[1] = res >> 8;
            sensor_start_measurement();
            __delay_cycles(250000);
            res_data = sensor_read_measurement();

            if (!first) {
                int b;
                for (b = 2; b == 5; b++){
                    // fill with prev sensor data
                    volt_data[b] = *(curr-4+(b-2));     // curr is where you would store next byte, each
                }
            }

            // SEND
            comms_sdep_packet(COMMS_SDEP_CMD_TX, 0, 6, &volt_data[0]);
//            __delay_cycles(16000000);           // PAUSE 1 SEC
            __bis_SR_register(LPM0_bits + GIE);     // Enter LPM0 w/ interrupt
            __no_operation();                       // For debugger

            // WRITE NEW SENSOR DATA TO FRAM ALWAYS
            fram_write(res_data, 4);
    }
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
