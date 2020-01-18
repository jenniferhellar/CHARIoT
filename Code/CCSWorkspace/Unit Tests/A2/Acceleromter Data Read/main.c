#include <msp430.h>
#include "p7_i2c.h"

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

void main(void)
{
    uint8_t *res_data;
    uint16_t sensor_data[6] = {0};
//    int 2s_comp_x;

    // Disable Watchdog Timer:
    WDTCTL = WDTPW | WDTHOLD;

    // Bump up clock speed:
    initClockTo16MHz();

    // Unlock pins:
    PM5CTL0 &= ~LOCKLPM5;

//    // Set debug LED pins to output direction
//    P6DIR |= 0x03;
//
//    // Turn LEDs off
//    P6OUT &= 0x00;

    // Globally enable interrupts:
    __bis_SR_register(GIE);

    // Call the code to initialize each subsystem, in order:
    p7_i2c_setup();

    // Setup 1khz timer A0:
    TA0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
    TA0CCR0 = 4000;
    TA0CTL = TASSEL__ACLK | ID_3 | MC__UP;        // ACLK, UP mode, 32khz/8 = 4khz

    uint8_t reg_addr[1] = {BMA2x2_X_AXIS_LSB_ADDR};
//    uint8_t reg_addr[2] = {0x29, 0x14};
    uint8_t success = 0;

    while(1) {
        // Main while loop:
        __bis_SR_register(LPM0_bits + GIE);     // Enter LPM0 w/interrupt (Timer A0 will wake)
        __no_operation();                       // For debugger

//        success = p7_i2c_write(ACCEL_I2C_ADDR, reg_addr, 2);
//        if(success > 0){
//            P6OUT |= 0x02;  // turn on LED and take a victory lap
//        }
        res_data = p7_i2c_reg_read(ACCEL_I2C_ADDR, &reg_addr[0], 1, 6);
        sensor_data[0] = *res_data;         // X LSB
        sensor_data[1] = *(res_data+1);
        sensor_data[2] = *(res_data+2);     // Y LSB
        sensor_data[3] = *(res_data+3);
        sensor_data[4] = *(res_data+4);     // Z LSB
        sensor_data[5] = *(res_data+5);

//        // Combining bytes to form 2's complement representation
//        2s_comp_x = ((int)(sensor_data[1]) << 4) + ((int)(sensor_data[0]) >> 4);
//        if(2s_comp_x & 0x0800) {
//            2s_comp_x |= 0xF000;
//        }
//        else {
//            2s_comp_x &= 0x0FFF;
//        }
//        // (decimal value)*(0.98mg/LSB)*(0.001 g/mg)*(9.806 m/s^2 per g)
//        if(sensor_data[0] > 0){
//            P6OUT |= 0x01;
//        }
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
