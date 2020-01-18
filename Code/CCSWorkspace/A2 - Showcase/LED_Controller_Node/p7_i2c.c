#include <msp430.h>
#include <stdint.h>
#include <stdlib.h>
#include "p7_i2c.h"

void p7_i2c_setup() {
    /*
     * Ports Used:
     *
     * P7.0 - UCB2SDA
     * P7.1 - UCB2SCL
     */

    /*
     * Setup eUSCI_B2 module for I2C communications:
     */

    // Hold the eUSCI_B2 module in reset state:
    UCB2CTLW0 |= UCSWRST;

    // I2C Master mode, SMCLK source:
    UCB2CTLW0 = UCMODE_3 | UCMST | UCSSEL__SMCLK | UCSYNC | UCSWRST;
    // Divide 16MHz clock by 256 (==62.5kHz))
    UCB2BRW = 256;

    // I2C pins
    P7SEL0 |= BIT0 | BIT1;
    P7SEL1 &= ~(BIT0 | BIT1);

    // Release software reset:
    UCB2CTLW0 &= ~UCSWRST;
}

#define P7_I2C_STATE_IDLE    0
#define P7_I2C_STATE_TX      1
#define P7_I2C_STATE_RX      2
#define P7_I2C_STATE_WAITING 3
static uint8_t state = P7_I2C_STATE_IDLE;
static uint8_t *data;
static unsigned int data_len = 0;
static unsigned int count = 0;
static uint8_t result = 0;
static unsigned int no_stop = 0;

uint8_t p7_i2c_write(uint8_t addr, uint8_t *tx_data, unsigned int len,
                     unsigned int no_stp) {
    // Temporarily disable GIE:
    __bic_SR_register(GIE);

    // Set peripheral address:
    UCB2I2CSA = addr & 0x7F;

    // Clear pending interrupts:
    UCB2IFG = 0;

    // Set state data:
    state = P7_I2C_STATE_TX;
    data = tx_data;
    data_len = len;
    count = 0;
    no_stop = no_stp;

    // Enable tx interrupt:
    UCB2IE |= UCTXIE0 | UCNACKIE;

    // Send I2C start condition and peripheral address:
    UCB2CTLW0 |= UCTR | UCTXSTT;

    // Enable GIE and drop into LPM0:
    __bis_SR_register(LPM0_bits | GIE);

    return result;
}

uint8_t *p7_i2c_read(uint8_t addr, unsigned int len) {
    // Temporarily disable GIE:
    __bic_SR_register(GIE);

    // Set peripheral address:
    UCB2I2CSA = addr & 0x7F;

    // Clear pending interrupts:
    UCB2IFG = 0;

    // Set state data:
    state = P7_I2C_STATE_RX;
    data = malloc(len);
    data_len = len;
    count = 0;

    // Enable rx interrupt:
    UCB2IE |= UCRXIE0 | UCNACKIE;

    // Send I2C start condition and peripheral address:
    UCB2CTLW0 &= ~UCTR;
    UCB2CTLW0 |= UCTXSTT;

    if (len == 1) {
        while (UCB2IFG & UCSTTIFG);
        UCB2CTLW0 |= UCTXSTP;
    }

    // Enable GIE and drop into LPM0:
    __bis_SR_register(LPM0_bits | GIE);

    return data;
}

#pragma vector = EUSCI_B2_VECTOR
__interrupt void p7_i2c_isr() {
    switch (state) {
        case P7_I2C_STATE_TX:
            if (!(UCB2IFG & UCNACKIFG) && (UCB2IFG & UCTXIFG0)) {
                // Ready for next data:
                if (count >= data_len) {
                    // Done transmitting, send STOP and disable module:
                    UCB2IE = 0;
                    if (no_stop) {
                        state = P7_I2C_STATE_WAITING;
                    } else {
                        UCB2CTLW0 |= UCTXSTP;
                        state = P7_I2C_STATE_IDLE;
                    }
                    result = 1;
                    __bic_SR_register_on_exit(LPM0_bits);
                } else {
                    UCB2TXBUF = *(data + count++);
                }
            } else {
                // NACK or unknown state, send STOP and disable module:
                UCB2IE = 0;
                UCB2CTLW0 |= UCTXSTP;
                state = P7_I2C_STATE_IDLE;
                result = 0;
                __bic_SR_register_on_exit(LPM0_bits);
            }
            break;
        case P7_I2C_STATE_RX:
            if (!(UCB2IFG & UCNACKIFG) && UCB2IFG & UCRXIFG0) {
                *(data + count++) = UCB2RXBUF;
                if (count == data_len - 1) {
                    // Stop after next byte:
                    UCB2CTLW0 |= UCTXSTP;
                } else if (count >= data_len) {
                    // Done transmitting, disable module:
                    if (count == 1) {
                        // Special case: 1-byte read, need to issue STP
                    }
                    UCB2IE = 0;
                    state = P7_I2C_STATE_IDLE;
                    __bic_SR_register_on_exit(LPM0_bits);
                }
            } else {
                // NACK or unknown state, send STOP and disable module:
                UCB2IE = 0;
                UCB2CTLW0 |= UCTXSTP;
                state = P7_I2C_STATE_IDLE;
                __bic_SR_register_on_exit(LPM0_bits);
            }
            break;
        default:
            // Module interrupting when it should not, disable interrupts.
            UCB2IE = 0;
            UCB2CTLW0 |= UCTXSTP;
            break;
    }
}


