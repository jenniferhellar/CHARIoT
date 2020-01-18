#include <msp430.h>
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

    // Enable pull-up resistors on SCL/SDA lines
    P7DIR &= ~(BIT0 | BIT1);
    P7OUT |= BIT0 | BIT1;
    P7REN |= BIT0 | BIT1;

    // Release software reset:
    UCB2CTLW0 &= ~UCSWRST;
}

/*
 * Code to transmit one byte. Called by the p7_i2c_read/write/reg_read functions
 * below.
 */
uint8_t tx_byte(uint8_t data, int i) {
    while (1) {
        __bis_SR_register(LPM0_bits | GIE);
        if (UCB2IFG & UCNACKIFG) {
            // If NACK, return failure.
            return 0;
        } else if (UCB2IFG & UCTXIFG0) {
            // Buffer ready for data:
            UCB2TXBUF = data;
            return 1;
        }
    }
}

/*
 * Code to receive one byte. Called by the p7_i2c_read/write/reg_read functions
 * below.
 */
uint8_t rx_byte(uint8_t *data) {
    while (1) {
        __bis_SR_register(LPM0_bits | GIE);
        if (UCB2IFG & UCNACKIFG) {
            // If NACK, return failure.
            return 0;
        } else if (UCB2IFG & UCRXIFG0) {
            // Buffer filled with data:
            *data = UCB2RXBUF;
            return 1;
        }
    }
}

// ISR for this code. All it does is wake up the main thread.
#pragma vector = EUSCI_B2_VECTOR
__interrupt void p7_i2c_isr() {
    __bic_SR_register_on_exit(LPM0_bits | GIE);
}

uint8_t p7_i2c_write(uint8_t addr, uint8_t *tx_data, unsigned int len) {
    int i;

    // Temporarily disable GIE:
    __bic_SR_register(GIE);

    // Set peripheral address:
    UCB2I2CSA = addr & 0x7F;

    // Clear pending interrupts:
    UCB2IFG = 0;

    // Send I2C start condition and peripheral address:
    UCB2CTLW0 |= UCTR | UCTXSTT;

    // Enable tx interrupt:
    UCB2IE = UCTXIE0 | UCNACKIE;
    for (i = 0; i < len; i++) {
        // Transmit all bytes:
        if (!tx_byte(*(tx_data + i), i)) {
            UCB2CTLW0 |= UCTXSTP;
            UCB2IE = 0;
            return 0;
        }
    }

    // Wait for last byte to actually start sending.
    while (1) {
        __bis_SR_register(LPM0_bits | GIE);
        if ((UCB2IFG & UCTXIFG0)||(UCB2IFG & UCNACKIFG)) {
            break;
        }
    }

    // While last byte is transmitting, issue stop command so I2C module
    // knows to stop after this byte.
    UCB2CTLW0 |= UCTXSTP;

    // Cleanup and return:
    UCB2IE = 0;
    __bis_SR_register(GIE);
    return 1;
}

uint8_t *p7_i2c_reg_read(uint8_t addr, uint8_t *tx_data, unsigned int tx_len,
                        unsigned int rx_len) {
    int i;
    uint8_t *data;

    // Temporarily disable GIE:
    __bic_SR_register(GIE);

    data = malloc(rx_len);

    // Set peripheral address:
    UCB2I2CSA = addr & 0x7F;

    // Clear pending interrupts:
    UCB2IFG = 0;

    // Send I2C start condition and peripheral address:
    UCB2CTLW0 |= UCTR | UCTXSTT;

    // Enable tx interrupt:
    UCB2IE = UCTXIE0 | UCNACKIE;
    for (i = 0; i < tx_len; i++) {
        // Transmit all bytes:
        if (!tx_byte(*(tx_data + i), i)) {
            UCB2CTLW0 |= UCTXSTP;
            UCB2IE = 0;
            return data;
        }
    }

    // Wait for last byte to actually start sending.
    while (1) {
        __bis_SR_register(LPM0_bits | GIE);
        if ((UCB2IFG & UCTXIFG0)||(UCB2IFG & UCNACKIFG)) {
            break;
        }
    }

    // Send I2C start condition and peripheral address:
    UCB2CTLW0 &= ~UCTR;
    UCB2CTLW0 |= UCTXSTT;

    // Wait for start condition to finish (necessary to ensure at least
    // one byte is read):
    while (UCB2CTLW0 & UCTXSTT);

    // Enable rx interrupt:
    UCB2IE = UCRXIE0 | UCNACKIE;
    for (i = 0; i < rx_len - 1; i++) {
        // Receive all but the last byte:
        if (!rx_byte(data + i)) {
            UCB2CTLW0 |= UCTXSTP;
            UCB2IE = 0;
            break;
        }
    }

    // While the last byte is being received, issue the stop command so
    // that the I2C module will stop after receiving this byte.
    UCB2CTLW0 |= UCTXSTP;

    // Receive the last byte:
    rx_byte(data + rx_len - 1);

    // Cleanup and return:
    UCB2IE = 0;
    __bis_SR_register(GIE);
    return data;
}

uint8_t *p7_i2c_read(uint8_t addr, unsigned int len) {
    int i;
    uint8_t *data;

    // Temporarily disable GIE:
    __bic_SR_register(GIE);

    data = malloc(len);

    // Set peripheral address:
    UCB2I2CSA = addr & 0x7F;

    // Clear pending interrupts:
    UCB2IFG = 0;

    // Send I2C start condition and peripheral address:
    UCB2CTLW0 &= ~UCTR;
    UCB2CTLW0 |= UCTXSTT;

    // Wait for start condition to finish (necessary to ensure at least
    // one byte is read):
    while (UCB2CTLW0 & UCTXSTT);

    // Enable rx interrupt:
    UCB2IE = UCRXIE0 | UCNACKIE;
    for (i = 0; i < len - 1; i++) {
        // Receive all but the last byte:
        if (!rx_byte(data + i)) {
            UCB2CTLW0 |= UCTXSTP;
            UCB2IE = 0;
            break;
        }
    }

    // While the last byte is being received, issue the stop command so
    // that the I2C module will stop after receiving this byte.
    UCB2CTLW0 |= UCTXSTP;

    // Receive the last byte:
    rx_byte(data + len - 1);

    // Cleanup and return:
    UCB2IE = 0;
    __bis_SR_register(GIE);
    return data;
}
