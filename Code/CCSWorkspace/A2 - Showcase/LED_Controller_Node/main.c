#include <msp430.h> 
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "comms_spi.h"
#include "pwr_control.h"
#include "p7_i2c.h"

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

static char tx_str[] = "cmd:reg:led";
static uint16_t tx_str_len = 11;

#define LED_CLK_OUT IO2_OUT
#define LED_DATA_OUT IO1_OUT
#define LED_CLK_DIR IO2_DIR
#define LED_DATA_DIR IO1_DIR
#define LED_CLK_BIT IO2_BIT
#define LED_DATA_BIT IO1_BIT

void send_spi_byte(uint8_t b) {
    int i;

    for (i = 7; i >= 0; i--) {
        LED_CLK_OUT &= ~LED_CLK_BIT;
        if ((b >> i) & 0x01) {
            LED_DATA_OUT |= LED_DATA_BIT;
        } else {
            LED_DATA_OUT &= ~LED_DATA_BIT;
        }
        __delay_cycles(100);
        LED_CLK_OUT |= LED_CLK_BIT;
    }
}

#define NUM_LEDS 23

void send_spi_color(uint8_t r, uint8_t g, uint8_t b) {
    int i;

    LED_CLK_OUT &= ~LED_CLK_BIT;
    LED_DATA_OUT &= ~LED_DATA_BIT;
    LED_CLK_DIR |= LED_CLK_BIT;
    LED_DATA_DIR |= LED_DATA_BIT;
    __delay_cycles(1000);
    LED_CLK_OUT |= LED_CLK_BIT;

    for (i = 0; i < 4; i++) {
        // Start frame
        send_spi_byte(0x00);
    }

    for (i = 0; i < NUM_LEDS; i++) {
        send_spi_byte(0xE4);
        send_spi_byte(b);
        send_spi_byte(g);
        send_spi_byte(r);
    }
    LED_CLK_OUT &= ~LED_CLK_BIT;
    LED_DATA_OUT &= ~LED_DATA_BIT;
}

int main(void)
{
    char *res_data;
    int res_len;
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

    while (1) {
        // Main while loop:

        res_data = comms_spi_write_read(tx_str, tx_str_len, &res_len);

        TA0R = 0;
        TA0CCR0 = 62500;
        TA0CCTL0 = CCIE;                        // TACCR0 interrupt enabled
        if (res_len == 3) {
            send_spi_color(res_data[0], res_data[1], res_data[2]);
            timer_counter = 5; // seconds between transmits
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
