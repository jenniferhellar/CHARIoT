#include <msp430.h> 
#include "p7_i2c.h"
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

void main(void)
{
    // Disable Watchdog Timer:
    WDTCTL = WDTPW | WDTHOLD;

    // Bump up clock speed:
    initClockTo16MHz();

    // Unlock pins:
    PM5CTL0 &= ~LOCKLPM5;

    // Set debug LED pins to output direction and turn off:
    P6DIR |= 0x03;
    P6OUT &= 0x00;

    // Globally enable interrupts:
    __bis_SR_register(GIE);

    // Call the code to initialize each subsystem, in order:
    p7_i2c_setup();
    accel_setup();

    // Setup complete, turn LED1 on:
    P6OUT |= 0x01;

    while(1) {
        // Main while loop:
        __bis_SR_register(LPM0_bits + GIE);     // Enter LPM0 w/interrupt (Accel will wake)
        __no_operation();                       // For debugger

        P6OUT ^= 0x03;                          // Toggle LEDs
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
  //P2IE  &= ~BIT3;   // Disable interrupt on P2.3
  P2IFG &= ~BIT3;   // No interrupt pending (don't want to trigger it again)
  __bic_SR_register_on_exit(LPM0_bits + GIE);  // Exit low power mode
}
